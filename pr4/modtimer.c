#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <asm/uaccess.h>
#include <linux/random.h>
#include <linux/workqueue.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/vmalloc.h>

#include "cbuffer.h"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("List module - FDI-UCM");
MODULE_AUTHOR("Alexis Cumbal Calderón");

//Buffer vars
#define BUFFER_LENGTH       PAGE_SIZE/8
#define MAX_CBUFFER_LEN		10
cbuffer_t* cbuffer;

// /proc-entry
static struct proc_dir_entry *proc_modtimer;
static struct proc_dir_entry *proc_modconfig;

//Timer and random config
/* Structure that describes the kernel timer */
struct timer_list my_timer;
unsigned long timer_period_ms = 1000;
int emergency_threshold = 80;
unsigned int max_random = 300;

//Spin-lock
DEFINE_SPINLOCK(sp); 
unsigned long flags;
int prod_count = 0; /* Número de procesos que abrieron la entrada
/proc para escritura (productores) */
int cons_count = 0; /* Número de procesos que abrieron la entrada
/proc para lectura (consumidores) */
struct semaphore mtx; /* para garantizar Exclusión Mutua */
struct semaphore sem_cons; /* cola de espera para consumidor(es) */
int nr_cons_waiting = 0; /* Número de procesos consumidores esperando */

//list
struct list_head mylist; /* linked list */
struct work_struct work; /* work struct */
//node(list)
typedef struct list_item_t {
    unsigned int data;
    struct list_head links;
};
//copy_items_into_list funct
static int copy_items_into_list(struct work_struct *work ){

    unsigned int elem;
    struct list_item_t *node;
    int size;

    spin_lock_irqsave( &sp, flags );
    size =  size_cbuffer_t( cbuffer );
    spin_unlock_irqrestore( &sp, flags );

    if( down_interruptible( &mtx ) ){
        return -EINTR;
    }

    while( size > 0 ){
        node = (struct list_item_t *)vmalloc(sizeof(struct list_item_t));
        
        spin_lock_irqsave( &sp, flags );
        elem = *(head_cbuffer_t(cbuffer));
        remove_cbuffer_t(cbuffer);
        spin_unlock_irqrestore( &sp, flags);

        node->data =  elem;

        list_add_tail( &node->links, &mylist );
        size--;
    }

    printk( KERN_INFO "We move %i elements from buffer to list\n", emergency_threshold/10);

    if( nr_cons_waiting > 0 ){
        up( &sem_cons );
        nr_cons_waiting--;
    }
    
    up( &mtx );
    return 0;
}
/* it is invoked when timer expires */
static void handle_timer(unsigned long data){
    unsigned int random =  (get_random_int()%max_random);
    int size, cpu_work, cpu_actual;
    printk( KERN_INFO "Generated random number: %u\n", random);

    spin_lock_irqsave( &sp, flags );
    insert_cbuffer_t( cbuffer, random );
    size = size_cbuffer_t( cbuffer );
    spin_unlock_irqrestore( &sp, flags );

    if( (size * 10) >= emergency_threshold ){
        cpu_actual = smp_processor_id();
        if( cpu_actual == 0 ){
            cpu_work = 1;
        }else{
            cpu_work = 0;
        }
        /*init work*/
        INIT_WORK( &work, copy_items_into_list );
        /*Enqueue work*/
        schedule_work_on( cpu_work, &work );
    }
    /*We reactivate time after a period of time (ms) from now*/
    mod_timer( &(my_timer), jiffies + timer_period_ms*(HZ/1000) );
}
//READ OPEN AND RELEASE FUNCTIONS -> TIMER
static ssize_t modtimer_read(struct file *filp, char __user *buf, size_t len, loff_t *off){
    
    struct list_item_t* tmp = NULL;
    struct list_head *pos, *q;
    char k_buf[BUFFER_LENGTH];
    char* dest =  k_buf;
    int i = 0;

    if( down_interruptible(&mtx) ){
        return -EINTR;
    }

    while( list_empty( &mylist ) ){
        nr_cons_waiting += 1;
        up( &mtx );
        if( down_interruptible( &sem_cons ) ){
            nr_cons_waiting -= 1;
            return -EINTR;
        }
        if( down_interruptible( &mtx ) ){
            return -EINTR;
        }
    }
    list_for_each_safe( pos, q, &mylist ){
        tmp = list_entry( pos, struct list_item_t, links );
        list_del(pos);
        printk( KERN_INFO "%u\n", tmp->data);
        dest += sprintf( dest, "%u\n", tmp->data );
        vfree(tmp);
    }
    
    up(&mtx);

    i = dest - k_buf;

    if( copy_to_user(buf, k_buf, i) ){
        return -EFAULT;
    }

    *off += i;
    return i;
}
static int modtimer_open(struct inode *inode, struct file *file){
    
    try_module_get(THIS_MODULE);
    //Init timer options
    my_timer.data = 0;
    my_timer.function = handle_timer;
    my_timer.expires = jiffies + timer_period_ms*(HZ/1000);
    //Activates timer once
    add_timer( &my_timer );
    return 0;
}
static int modtimer_release(struct inode *inode, struct file *file){
   
    struct list_item_t* tmp = NULL;
    struct list_head *pos, *q;

    //Delete timer
    del_timer_sync( &my_timer );
    //we have to wait 4 pending tasks //http://www.makelinux.net/books/lkd2/ch07lev1sec4
    flush_schedule_work(); //***********OJO********

    //Cleaning buffer
    spin_lock_irqsave( &sp, flags );
    clear_cbuffer_t( cbuffer );
    spin_unlock_irqrestore( &sp, flags);

    list_for_each_safe( pos, q, &mylist ){
        tmp = list_entry( pos, struct list_item_t, links );
        list_del( pos );
        vfree( tmp );
    }

    module_put(THIS_MODULE);

    return 0;

}
static const struct file_operations proc_entry_modtimer = {
    .read = modtimer_read, 
    .open 	 = modtimer_open,
	.release = modtimer_release,    
};
//READ OPEN AND RELEASE FUNCTIONS -> CONFIG 
static ssize_t modconfig_read(struct file *filp, char __user *buf, size_t len, loff_t *off){

    static int is_finished = 0;
    char k_buffer[BUFFER_LENGTH];
    int n;

    if(is_finished){
        is_finished = 0;
    }

    n = sprintf( k_buffer, "timer_period_ms=%lu\nemergency_threshold=%i\nmax_random=%i\n", timer_period_ms, emergency_threshold, max_random );

    is_finished = 1;

    //Has perdido algo en mi pantalla?
    if( copy_from_user( buf, k_buffer, len) ){
        return -EFAULT;
    }

    return n;
}
/*
Lee desde el usuario los parámetros de configuración del módulo
*/
static ssize_t modconfig_write(struct file *filp, const char __user *buf, size_t len, loff_t *off){
    
    unsigned long number1;
    int number2;
    char k_buffer[BUFFER_LENGTH];

    if( copy_from_user( k_buffer, buf, len) ){
        return -EFAULT;
    }

    if( sscanf( k_buffer,  "Mili seconds timer period %lu", &number1 ) == 1 ){
        timer_period_ms = number1;
        printk(KERN_INFO "%lu\n", timer_period_ms);
    }

    if( sscanf(k_buffer, "Emergency threshold %i", &number2) == 1 ){
        if( number2 <= 100 ){
            emergency_threshold = number2;
            printk(KERN_INFO "%i\n", emergency_threshold);
        }else{
            printk(KERN_INFO "Emergency threshold must be between 0 and 100 \n");
        }
    }

    if( sscanf( k_buffer, "Maximun random number %i", &number2 ) == 1 ){
        max_random = number2;
        printk(KERN_INFO "%i\n", max_random);
    }
    
    return len;

}
static const struct file_operations proc_entry_modconfig = {
    .read = modconfig_read,
    .write = modconfig_write,    
};

/* Función que se invoca cuando se carga el módulo en el kernel */
static int  __init modulo_lin_init(void){
    //init semaphore
    sema_init(&mtx,1);
    sema_init(&sem_cons,0);
    //init list_head
    INIT_LIST_HEAD(&mylist);
    cbuffer = create_cbuffer_t(MAX_CBUFFER_LEN);
    //init proc entries
    proc_modconfig = proc_create("modconfig", 0666, NULL, &proc_entry_modconfig);
    proc_modtimer = proc_create("modtimer", 0666, NULL, &proc_entry_modtimer);
    //init timer
    init_timer(&my_timer);

    if( proc_modconfig == NULL ||
        proc_modtimer == NULL ){
            printk(KERN_INFO "Cannot create /proc/* entries");
            return -ENOMEM;
    }else{
        printk(KERN_INFO "Modulo cargado");
    }
    return 0;
}
/* Función que se invoca cuando se descarga el módulo del kernel */
static void __exit modulo_lin_clean(void){
    destroy_cbuffer_t(cbuffer);
    del_timer_sync(&my_timer);
    remove_proc_entry("modtimer", NULL);
    remove_proc_entry("modconfig", NULL);
    printk(KERN_INFO "modtimer: Module unloaded.\n");
    printk(KERN_INFO "modconfig: Module unloaded.\n");
}
/* Declaración de funciones init y cleanup */
module_init(modulo_lin_init);
module_exit(modulo_lin_clean);
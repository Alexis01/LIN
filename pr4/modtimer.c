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

#define BUFFER_LENGTH       PAGE_SIZE/8
#define MAX_CBUFFER_LEN     10

static struct proc_dir_entry *proc_modtimer;
static struct proc_dir_entry *proc_modconfig;

struct timer_list my_timer; /* Structure that describes the kernel timer */

unsigned long timer_period_ms = 1000;
int emergency_threshold = 80; 
unsigned int max_random = 300;

DEFINE_SPINLOCK(sp); //Spin-lock

cbuffer_t* cbuffer;


unsigned long flags;
int prod_count = 0; /* Número de procesos que abrieron la entrada /proc para escritura (productores) */
int cons_count = 0; /* Número de procesos que abrieron la entrada /proc para lectura (consumidores) */
struct semaphore mtx; /* para garantizar Exclusión Mutua */
struct semaphore sem_cons; /* cola de espera para consumidor(es) */
int nr_cons_waiting=0; /* Número de procesos consumidores esperando */
struct list_head mylist; /* Lista enlazada */
struct work_struct work;

/* Nodos de la lista */
struct list_item_t {
    unsigned int data;
    struct list_head links;
} list_item_t;


static int copy_items_into_list(struct work_struct *work ){
    //unsigned int elem;
    unsigned char elem;
    struct list_item_t *node;
    int size;
    
    spin_lock_irqsave(&sp,flags);
    size = size_cbuffer_t(cbuffer);
    spin_unlock_irqrestore(&sp,flags);
        
    if (down_interruptible(&mtx)){
        return -EINTR;
    }
    
    while(size > 0){
        
        node = (struct list_item_t *)vmalloc(sizeof(struct list_item_t));
        
        spin_lock_irqsave(&sp,flags);
        elem = *(head_cbuffer_t(cbuffer));
        remove_cbuffer_t(cbuffer);
        spin_unlock_irqrestore(&sp,flags);
        
        
        node->data = elem;
        
        list_add_tail(&node->links, &mylist);
        
        size--;
    }
    
    printk(KERN_INFO "We are going to move %i elements from buffer to the list", emergency_threshold/10);
    
    if(nr_cons_waiting > 0){
        up(&sem_cons);
        nr_cons_waiting--;
    }
    
    up(&mtx);
    return 0;
}

/* Manejador del timer */
static void timer_handler(unsigned long data){
    unsigned int random = (get_random_int()%max_random);
    //unsigned char ch;
    int size, cpu_work, cpu_actual;
    printk(KERN_INFO "Generate Number: %u\n", random);
    //memcpy(ch, (char*)&random, 2);
    //Insert random number into cbuffer
    spin_lock_irqsave(&sp,flags);
    //insert_cbuffer_t(cbuffer,ch);
    insert_cbuffer_t(cbuffer,random);
    size = size_cbuffer_t(cbuffer);
    spin_unlock_irqrestore(&sp,flags);
    
    if(size*10 >= emergency_threshold){
        cpu_actual = smp_processor_id();
        if(cpu_actual == 0){
            cpu_work = 1;
        }
        else{
            cpu_work = 0;
        }
            
        INIT_WORK(&work, copy_items_into_list);
        
        /* Enqueue work */
        schedule_work_on(cpu_work,&work);
    }       
                        
     /* Reactivamos el timer tras el periodo configurado */
    mod_timer( &(my_timer), jiffies + timer_period_ms*HZ/1000); 
}

static ssize_t modtimer_read(struct file *filp, char __user *buf, size_t len, loff_t *off){
    
    struct list_item_t* tmp=NULL;
    struct list_head *pos, *q;
    char kbuf[BUFFER_LENGTH];
    char* dest=kbuf;    
    int i = 0;
    if (down_interruptible(&mtx)){  
        return -EINTR;  
    }
    
    while(list_empty(&mylist)){
        nr_cons_waiting +=1;
        up(&mtx);
        if (down_interruptible(&sem_cons)){
            nr_cons_waiting --;
            return -EINTR;  
        }
        if (down_interruptible(&mtx)){  
            return -EINTR;  
        }
    }
    
    list_for_each_safe(pos, q, &mylist){
        tmp= list_entry(pos, struct list_item_t, links);
        list_del(pos);
        printk(KERN_INFO "%u\n",tmp->data); 
        dest+=sprintf(dest,"%u\n",tmp->data);
        vfree(tmp); 
    }
    up(&mtx);
    
    i=dest-kbuf;
        
    if (copy_to_user(buf, kbuf,i))
        return -EFAULT;
    *off += i;
    return i;
 
}

static int modtimer_open(struct inode *inode, struct file *file){
    try_module_get(THIS_MODULE);
    
    /* Initialize field */
    my_timer.data=0;
    my_timer.function=timer_handler;
    my_timer.expires=jiffies + timer_period_ms*(HZ/1000);
    /* Activate the timer for the first time */
    add_timer(&my_timer);
    
    return 0;
}

static int modtimer_release(struct inode *inode, struct file *file){
    
    struct list_item_t* tmp=NULL;
    struct list_head *pos, *q;
    
    //Borramos el timer
    del_timer_sync(&my_timer);
    //Eperamos a la cola de trabajo
    flush_scheduled_work();
    
    //Vaciamos el buffer
    spin_lock_irqsave(&sp,flags);
    clear_cbuffer_t(cbuffer);
    spin_unlock_irqrestore(&sp,flags);
    
    list_for_each_safe(pos, q, &mylist){
         tmp= list_entry(pos, struct list_item_t, links);
         list_del(pos);
         vfree(tmp);
    }
    
    module_put(THIS_MODULE);
    return 0;
}


static const struct file_operations proc_entry_modtimer = {
    .read = modtimer_read, 
    .open    = modtimer_open,
    .release = modtimer_release,    
};


static ssize_t modconfig_read(struct file *filp, char __user *buf, size_t len, loff_t *off){
    
    static int finished = 0;
    char kbuf[BUFFER_LENGTH];
    int n;
    
    if (finished) {
            finished = 0;
            return 0;
    }

    n=sprintf(kbuf,"timer_period_ms=%lu\nemergency_threshold=%i\nmax_random=%i\n",timer_period_ms, emergency_threshold, max_random);
    
    finished =1;
    
    if (copy_to_user(buf, kbuf,n))
        return -EFAULT;
        
    return n;    
}

static ssize_t modconfig_write(struct file *filp, const char __user *buf, size_t len, loff_t *off){
    unsigned long num;
    int num2;
    char kbuf[BUFFER_LENGTH];
    
    if (copy_from_user(kbuf, buf, len))
        return -EFAULT;

    if (sscanf(kbuf, "timer_period_ms %lu", &num) == 1){
        timer_period_ms = num;
        printk(KERN_INFO "MODTIMER: new timer period assigned: %lu\n",timer_period_ms); 
    }
    
    if (sscanf(kbuf, "emergency_threshold %i", &num2) == 1){
        if (num2 <= 100){
            emergency_threshold = num2;
            printk(KERN_INFO "MODTIMER: new timer emergency threshold assigned: %i\n",emergency_threshold); 
        }else{
            printk(KERN_ALERT "MODTIMER: to high value for this param (max 100): \n");
        }
    }
    
    if (sscanf(kbuf, "max_random %i", &num2) == 1){
        max_random = num2;
        printk(KERN_INFO "MODTIMER: new timer Number limit: %i\n",max_random);  
    }
    return len;
}



static const struct file_operations proc_entry_modconfig = {
    .read = modconfig_read,
    .write = modconfig_write,    
};


/* Función que se invoca cuando se carga el módulo en el kernel */
int modtimer_init(void){
    sema_init(&mtx,1);
    sema_init(&sem_cons,0);
    
    INIT_LIST_HEAD(&mylist); //inicializamos la lista
    cbuffer = create_cbuffer_t (MAX_CBUFFER_LEN);
    
    proc_modconfig = proc_create("modconfig", 0666, NULL, &proc_entry_modconfig);
    proc_modtimer = proc_create("modtimer", 0666, NULL, &proc_entry_modtimer);

    /* Create timer */
    init_timer(&my_timer);

    if (proc_modconfig == NULL || proc_modtimer == NULL) {
        printk(KERN_INFO "MODTIMER: Cannot load this module \n");
        return -ENOMEM;     
    } 
    else{
         printk(KERN_INFO "MODTIMER: Module loaded sucessfully!\n");
    }            
    return 0;
}
void modtimer_clean(void){

    destroy_cbuffer_t(cbuffer);
    
    del_timer_sync(&my_timer);
    remove_proc_entry("modtimer", NULL);
    remove_proc_entry("modconfig", NULL);
 
    printk(KERN_INFO "MOTIMER: Module released!\n");
}

module_init(modtimer_init);
module_exit(modtimer_clean);
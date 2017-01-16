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

#include "cbuffer.h"

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
//list
struct list_head mylist; /* linked list */
struct work_struct work; /* work struct */
//node(list)
typedef struct list_item_t {
    unsigned int data;
    struct list_head links;
}
//copy_items_into_list funct
static int copy_items_into_list(struct work_struct *work ){

}
/* it is invoked when timer expires */
static void fire_timer(unsigned long data){

}
//READ OPEN AND RELEASE FUNCTIONS -> TIMER
static ssize_t modtimer_read(struct file *filp, char __user *buf, size_t len, loff_t *off){

}
static int modtimer_open(struct inode *inode, struct file *file){

}
static int modtimer_release(struct inode *inode, struct file *file){

}
static const struct file_operations proc_entry_modtimer = {
    .read = modtimer_read, 
    .open 	 = modtimer_open,
	.release = modtimer_release,    
};
//READ OPEN AND RELEASE FUNCTIONS -> CONFIG 
static ssize_t modconfig_read(struct file *filp, char __user *buf, size_t len, loff_t *off){

}
static ssize_t modconfig_write(struct file *filp, const char __user *buf, size_t len, loff_t *off){

}
static const struct file_operations proc_entry_modconfig = {
    .read = modconfig_read,
    .write = modconfig_write,    
};
/* Función que se invoca cuando se carga el módulo en el kernel */
int modulo_lin_init(void){
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
void modulo_lin_clean(void){
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
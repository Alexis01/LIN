/*
 *  Module that implements a list
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/vmalloc.h>
#include <linux/ftrace.h>
#include <linux/proc_fs.h>
#include <asm-generic/uaccess.h>
#include "cbuffer.c"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("List module - FDI-UCM");
MODULE_AUTHOR("Alexis Cumbal Calderón");

#define BUFFER_LENGTH       PAGE_SIZE/8

/******Resources*********/
//buffer
cbuffer_t* cbuffer; /* Buffer circular */
//Dos contadores
int prod_count = 0;/* Número de procesos que abrieron la entrada /proc para escritura (productores) */
int cons_count = 0;/* Número de procesos que abrieron la entrada /proc para lectura (consumidores) */
//Mutex para el buffer y los contadores
struct semaphore mtx;/* para garantizar Exclusión Mutua */
struct semaphore sem_prod; /* cola de espera para productor(es) */
truct semaphore sem_cons; /* cola de espera para consumidor(es) */
//Dos variables condicionales
int nr_prod_waiting=0; /* Número de procesos productores esperando */
int nr_cons_waiting=0; /* Número de procesos consumidores esperando */



static struct proc_dir_entry *proc_entry;
static char *modlist;  // Space for the "modlist"



static const struct file_operations proc_entry_fops = {
	.read = fifoproc_read,
	.write = fifoproc_write,
    .open = fifoproc_open,
    .release = fifoproc_release
};

int createEntry(void){
    int ret = 0;
	
	modlist = (char *) vmalloc( BUFFER_LENGTH );
	if(!modlist){
		ret = -ENOMEM;
	}else{
		memset( modlist, 0, BUFFER_LENGTH );
		proc_entry = proc_create( "modlist", 0666, NULL, &proc_entry_fops );
		if( proc_entry == NULL ){
			ret = -ENOMEM;
			vfree( modlist );
			printk(KERN_INFO "Modlist: Can't create /proc/modlist entry\n");
		}else{
			printk(KERN_INFO "Modlist: Creates /proc/modlist entry\n");
		} 
	}
	return ret;
}
/* Se invoca al hacer open() de entrada /proc */
static int fifoproc_open(struct inode *, struct file *){
	if(file->f_mode & FMODE_READ){
		/* Un consumidor abrió el FIFO */
	
	}else{
		/* Un productor abrió el FIFO */
	
	}
}
/* Se invoca al hacer close() de entrada /proc */
static int fifoproc_release(struct inode *, struct file *){

}
/* Se invoca al hacer read() de entrada /proc */
static ssize_t fifoproc_read(struct file *, char *, size_t, loff_t *){

}
/* Se invoca al hacer write() de entrada /proc */
static ssize_t fifoproc_write(struct file *, const char *, size_t, wloff_t *){

}
void fifoproc_open(bool abre_para_lectura) {
	/* Completar */

}
int fifoproc_write(char* buff, int len) {
	/* Completar */
	char kbuffer[MAX_KBUF];

	if (len> MAX_CBUFFER_LEN || len> MAX_KBUF) { 
		return Error;
	}

	if (copy_from_user(kbuffer,buff,len)) { 
		return Error;
	}

	lock(mtx);
	/* Esperar hasta que haya hueco para insertar (debe haber consumidores) */
	while (nr_gaps_cbuffer_t(cbuffer)<len && cons_count>0){
		cond_wait(prod,mtx);
	}
	/* Detectar fin de comunicación por error (consumidor cierra FIFO antes) */
	if (cons_count==0) {
		unlock(mtx); 
		return -EPIPE;
	}
	insert_items_cbuffer_t(cbuffer,kbuffer,len);
	/* Despertar a posible consumidor bloqueado */
	cond_signal(cons);
	
	unlock(mtx);
	
	return len;
}
int fifoproc_read(const char* buff, int len) {
	/* Completar */

}
void fifoproc_release(bool lectura) {
	/* Completar */

}
static int __init init_module(void){
	
    if( createEntry() != 0 ){
        printk(KERN_INFO "Cannot create entry!!\n");
    	return -ENOSPC;
    }else{
    	INIT_LIST_HEAD(&my_list);// ‘struct list_item’ has no member named ‘list’
		printk(KERN_INFO "ListMod: Loaded\n");
	    return 0;
    }
	
}




static void __exit cleanup_module(void){

	remove_proc_entry("modlist", NULL);
	vfree( modlist );
	printk(KERN_INFO "ListMod: Removed\n");
}

module_init(list_module_init);
module_exit(list_module_exit);




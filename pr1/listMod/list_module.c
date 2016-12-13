/*
 *  hello-5.c - Demonstrates command line argument passing to a module.
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
#include "list.h"


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("List module - FDI-UCM");
MODULE_AUTHOR("Alexis Cumbal Calderón");

#define BUFFER_LENGTH       PAGE_SIZE
static int myint = -1;
module_param(myint, int, 0000);

struct list_item {
	int data;
	struct list_head links;
};
struct list_head my_list;

void add_node_list(int myint) { 
	struct list_item *first;
	first = (struct list_item *)vmalloc(sizeof(struct list_item));
	first->data = myint;
	list_add_tail(&first->links, &my_list);
}
void print_list(struct list_head* list) {
	struct list_item* item=NULL;
	struct list_head* cur_node=NULL;
	struct list_head* aux=NULL;
	list_for_each_safe(cur_node,aux,list) {
		/* item points to the structure wherein the links are embedded */
		item = list_entry(cur_node, struct list_item, links);
		printk(KERN_INFO "%i\n",item->data);
	}
}
void remove_node_list(int myint, struct list_head *list) { 
	struct list_item* item=NULL;
	struct list_head* cur_node=NULL;
	struct list_head* aux=NULL;
	list_for_each_safe(cur_node,aux,list) {
		/* item points to the structure wherein the links are embedded */
		item = list_entry(cur_node, struct list_item, links);
		if( item->data == myint ){
			list_del( &item->links );
			vfree( item );
		}
		
	}
}
void cleanup_list( struct list_head *list ) { 
	struct list_item* item=NULL;
	struct list_head* cur_node=NULL;
	struct list_head* aux=NULL;
	list_for_each_safe(cur_node,aux,list) {
		/* item points to the structure wherein the links are embedded */
		item = list_entry(cur_node, struct list_item, links);
		list_del( &item->links );
		vfree( item );
	}
}
//Entrada 
#define BUFFER_LENGTH       PAGE_SIZE
static struct proc_dir_entry *proc_entry;
static char *modlist;  // Space for the "modlist"


static ssize_t modlist_write ( struct file *filp, const char __user *buf, size_t len, loff_t *off  ){
	char orden[20];
	int available_space = BUFFER_LENGTH - 1;
	if( (*off) > 0 ){
		printk(KERN_INFO "The apstatic int myintArray[2] = { -1, -1 };plication can write in this entry just once !!\n");
		return 0;
	}
	if( len > available_space ){
		printk(KERN_INFO "modlist: not enough space!!\n");
    	return -ENOSPC;
	}
	/*Pasamos datos desde el espacio de usuario
		al espacio de kernel */
	if( copy_from_user( &modlist[0], buf, len ) ){
		return -EFAULT;
	}

	modlist[ len ] = '\0'; /*añadimos fin de escritura*/
	
	
	//sscanf( dtm, "%s %s %d  %d", weekday, month, &day, &year );
	
    
	sscanf(modlist, "%s %d",orden,&myint);
	if (strcmp(orden,"add") == 0){
		
		add_node_list(myint);
		print_list(&my_list);
		
	}
	if (strcmp(orden,"remove") == 0){
		
		remove_node_list(myint,&my_list);
		print_list(&my_list);
	}
	//
	if (strcmp(orden,"cleanup") == 0){
		
		cleanup_list(&my_list);
		print_list(&my_list);
	}

	*off += len; /* actualizamos el puntero del fichero */

	return len;

}


static ssize_t modlist_read ( struct file *filp, char __user *buf, size_t len, loff_t *off ){
	int nr_bytes;
	struct list_item* item=NULL;
	struct list_head* cur_node=NULL;
	struct list_head* aux=NULL;
	char buff_kern[255];
	char *dest = buff_kern;
	//TODO ACUMULAR EN UN BUFFER 
	if( (*off) > 0 ){
		printk(KERN_INFO "Tell the application that there is nothing left to read !!\n");
		return 0;
	}
	
	//

	
	list_for_each_safe(cur_node,aux,&my_list) {
		/* item points to the structure wherein the links are embedded */
		item = list_entry(cur_node, struct list_item, links);
		dest += sprintf(dest,"%i\n",item->data);
	}

	nr_bytes = strlen( buff_kern );

	if( len < nr_bytes ){
		return -ENOSPC;
	}

	/*Pasamos datos desde el espacio de kernel
		al espacio de usuario */
	if( copy_to_user( buf, buff_kern, nr_bytes ) ){
		return -EINVAL;
	}
	(*off) += nr_bytes;



	return nr_bytes;

}

static const struct file_operations proc_entry_fops = {
	.read = modlist_read,
	.write = modlist_write,
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
			printk(KERN_INFO "Modlist: Module loaded\n");
		} 
	}
	return ret;
}


static int __init list_module_init(void){
	printk(KERN_INFO "ListMod: Loaded\n");
    if( createEntry() != 0 ){
        printk(KERN_INFO "Cannot create entry!!\n");
    	return -ENOSPC;
    }else{
    	INIT_LIST_HEAD(&my_list);// ‘struct list_item’ has no member named ‘list’
	    return 0;
    }
	
}




static void __exit list_module_exit(void){
    remove_proc_entry("modlist", NULL);
	vfree( modlist );
	
	printk(KERN_INFO "ListMod: Removed\n");
}

module_init(list_module_init);
module_exit(list_module_exit);





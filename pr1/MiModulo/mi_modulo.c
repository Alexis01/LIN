#include <linux/module.h>	/* Requerido por todos los módulos */
#include <linux/kernel.h>	/* Definición de KERN_INFO */
#include <linux/ftrace.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <asm-generic/uaccess.h>
MODULE_LICENSE("GPL"); 	/*  Licencia del modulo */
MODULE_AUTHOR("******Alexis******");


/********Inicialización*************/
struct list_head mylist; /* Lista enlazada */
/* Nodos de la lista */
typedef struct {
	int data;
	struct list_head links;
}list_item_t;
//Entrada 
#define BUFFER_LENGTH       PAGE_SIZE
static struct proc_dir_entry *proc_entry;
static char *modlist;  // Space for the "clipboard"

static ssize_t modlist_write ( struct file *filp, const char __user *buf, size_t len, loff_t *off  ){
	int available_space = BUFFER_LENGTH - 1;
	if( (*off) > 0 ){
		printk(KERN_INFO "The application can write in this entry just once !!\n");
		return 0;
	}
	if( len > available_space ){
		printk(KERN_INFO "clipboard: not enough space!!\n");
    	return -ENOSPC;
	}
	/*Pasamos datos desde el espacio de usuario
		al espacio de kernel */
	if( copy_from_user( &modlist[0], buf, len ) ){
		return -EFAULT;
	}

	modlist[ len ] = '\0'; /*añadimos fin de escritura*/
	*off += len; /* actualizamos el puntero del fichero */
	trace_printk("Current value of modlist is : %s\n",modlist);

	return len;

}

static ssize_t modlist_read ( struct file *filp, char __user *buf, size_t len, loff_t *off ){
	int nr_bytes;
	if( (*off) > 0 ){
		printk(KERN_INFO "Tell the application that there is nothing left to read !!\n");
		return 0;
	}
	
	nr_bytes = strlen( modlist );
	if( len < nr_bytes ){
		return -ENOSPC;
	}

	/*Pasamos datos desde el espacio de kernel
		al espacio de usuario */
	if( copy_to_user( buf, modlist, nr_bytes ) ){
		return -EINVAL;
	}
	(*off) += len;

	return nr_bytes;

}

static const struct file_operations proc_entry_fops = {
	.read = modlist_read,
	.write = modlist_write,
};


/* Función que se invoca cuando se carga el módulo en el kernel */
int modulo_lin_init(void){
	/* Devolver 0 para indicar una carga correcta del módulo */
	int ret = 0;

	printk(KERN_INFO "Modulo LIN cargado. Hola kernel.\n");
	trace_printk("modulo ON\n");
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

/* Función que se invoca cuando se descarga el módulo del kernel */
void modulo_lin_clean(void){
	remove_proc_entry("modlist", NULL);
	vfree( modlist );
	printk(KERN_INFO "Modulo LIN descargado. Adios kernel.\n");
	trace_printk("modulo OFF\n");
}

/* Declaración de funciones init y exit */
module_init(modulo_lin_init);
module_exit(modulo_lin_clean);


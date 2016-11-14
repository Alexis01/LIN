#include <linux/module.h> 
#include <asm-generic/errno.h>
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/tty.h>      /* For fg_console */
#include <linux/kd.h>       /* For KDSETLED */
#include <linux/vt_kern.h>



MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("List module - FDI-UCM");
MODULE_AUTHOR("Alexis Cumbal Calderón");

#define BUFFER_LENGTH       PAGE_SIZE
#define ALL_LEDS_ON 0x7
#define ALL_LEDS_OFF 0

//Entrada 
static struct proc_dir_entry *proc_entry;
static char *modlist;  // Space for the "modlist"
struct tty_driver* kbd_driver= NULL;


//MANEJADOR
/* Get driver handler */
struct tty_driver* get_kbd_driver_handler(void){
   printk(KERN_INFO "modleds: loading\n");
   printk(KERN_INFO "modleds: fgconsole is %x\n", fg_console);
   return vc_cons[fg_console].d->port.tty->driver;
}

/* Set led state to that specified by mask */
static inline int set_leds(struct tty_driver* handler, unsigned int mask){
    return (handler->ops->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED,mask);
}

static ssize_t modlist_write ( struct file *filp, const char __user *buf, size_t len, loff_t *off  ){
	unsigned int mask;
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
	
    kbd_driver= get_kbd_driver_handler();
	sscanf(modlist, "0x%x", &mask);
	set_leds( kbd_driver, mask ); 
	

	*off += len; /* actualizamos el puntero del fichero */

	return len;

}


static ssize_t modlist_read ( struct file *filp, char __user *buf, size_t len, loff_t *off ){
	int nr_bytes;
	char buff_kern[255];
	//char *dest = buff_kern;

	if( (*off) > 0 ){
		printk(KERN_INFO "Tell the application that there is nothing left to read !!\n");
		return 0;
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
		proc_entry = proc_create( "modleds", 0666, NULL, &proc_entry_fops );
		if( proc_entry == NULL ){
			ret = -ENOMEM;
			vfree( modlist );
			printk(KERN_INFO "Modlist: Can't create /proc/modleds entry\n");
		}else{
			printk(KERN_INFO "Modlist: Module loaded\n");
		} 
	}
	return ret;
}


static int __init list_module_init(void){
	kbd_driver= get_kbd_driver_handler();
    if( createEntry() != 0 ){
        printk(KERN_INFO "Cannot create entry!!\n");
    	return -ENOSPC;
    }else{
    	//hemos creado la entrada
    	kbd_driver= get_kbd_driver_handler();
    	printk(KERN_INFO "ListMod: Loaded\n");
    	
	    return 0;
    }
	
}




static void __exit list_module_exit(void){
    remove_proc_entry("modlist", NULL);
    set_leds(kbd_driver,ALL_LEDS_OFF);
	printk(KERN_INFO "ListMod: Removed\n");
}

module_init(list_module_init);
module_exit(list_module_exit);



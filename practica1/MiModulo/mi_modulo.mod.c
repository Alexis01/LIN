#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xc6c01fa, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x5dfa304a, __VMLINUX_SYMBOL_STR(remove_proc_entry) },
	{ 0x999e8297, __VMLINUX_SYMBOL_STR(vfree) },
	{ 0xa13aece1, __VMLINUX_SYMBOL_STR(proc_create_data) },
	{ 0xd6ee688f, __VMLINUX_SYMBOL_STR(vmalloc) },
	{ 0xb1e25684, __VMLINUX_SYMBOL_STR(__trace_bputs) },
	{ 0x754d539c, __VMLINUX_SYMBOL_STR(strlen) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xc72e1233, __VMLINUX_SYMBOL_STR(__trace_bprintk) },
	{ 0x69acdf38, __VMLINUX_SYMBOL_STR(memcpy) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "73D8474B130EEACA7A08A56");

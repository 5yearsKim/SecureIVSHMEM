/*
 * copyright: Lee, Jerry J (jerry.j.lee@intel.com)
 */

#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/module.h>

static int __init _ivshmem_uio_hook_init(void) {
        printk(KERN_INFO "ivshmem_uio_hook: module loaded\n");
        return 0;
}

static void __exit _ivshmem_uio_hook_exit(void) {
        printk(KERN_INFO "ivshmem_uio_hook: module unloaded\n");
}

module_init(_ivshmem_uio_hook_init);
module_exit(_ivshmem_uio_hook_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lee, Jerry J");
MODULE_DESCRIPTION("PCI UIO generic driver hooking module for the ivshmem.");

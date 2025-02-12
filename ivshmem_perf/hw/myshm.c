/*
 * myshm.c - A simple character device driver with 4MB memory and mmap support.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>           // alloc_chrdev_region, file_operations
#include <linux/cdev.h>         // cdev utilities
#include <linux/device.h>       // class_create, device_create
#include <linux/vmalloc.h>      // vmalloc, vfree, vmalloc_to_pfn
#include <linux/mm.h>           // remap_pfn_range, PAGE_SIZE
#include <linux/uaccess.h>      // copy_to_user, etc.
#include <linux/string.h>       // memset

#define DEVICE_NAME "myshm"
#define CLASS_NAME  "shm_class"
#define MEM_SIZE    (4 * 1024 * 1024)  // 4MB

static int major;                        // Major number allocated to our device
static dev_t dev_number;
static struct class *mmap_class = NULL;  // Device class
static struct cdev my_cdev;              // Character device structure

/* 
 * Use a variable for the printk prefix.
 * This allows easy modification in the future.
 */
static const char *shm_prefix = "myshm: ";

/* 
 * File operations
 */

// shm_open: Called when the device file is opened.
static int shm_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "%sDevice opened\n", shm_prefix);
    return 0;
}

// shm_release: Called when the device file is closed.
static int shm_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "%sDevice closed\n", shm_prefix);
    return 0;
}

// /*
//  * shm_mmap: Maps our 4MB kernel buffer into the user process’s address space.
//  *
//  * The user process will call mmap() on the device file, and this function
//  * will map the vmalloc()’ed memory into its address space page by page.
//  */
// static int shm_mmap(struct file *file, struct vm_area_struct *vma)
// {
//     unsigned long user_start = vma->vm_start;
//     unsigned long size = vma->vm_end - vma->vm_start;
//     unsigned long offset = 0;
//     unsigned long pfn;
//     int ret;

//     if (size > MEM_SIZE) {
//         printk(KERN_ERR "%sRequested mmap size too large\n", shm_prefix);
//         return -EINVAL;
//     }

//     /* Map each page from our device_buffer into user space */
//     while (offset < size) {
//         /* Convert virtual address (within device_buffer) to page frame number */
//         pfn = vmalloc_to_pfn(device_buffer + offset);
//         ret = remap_pfn_range(vma, user_start + offset, pfn, PAGE_SIZE, vma->vm_page_prot);
//         if (ret < 0) {
//             printk(KERN_ERR "%sremap_pfn_range failed at offset %lu\n", shm_prefix, offset);
//             return ret;
//         }
//         offset += PAGE_SIZE;
//     }
//     printk(KERN_INFO "%smmap successful, mapped %lu bytes\n", shm_prefix, size);
//     return 0;
// }

// Define file operations that our driver supports.
static struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = shm_open,
    .release = shm_release,
};

/*
 * Shared memory device buffer.
 * Note: This variable is used in shm_mmap and is allocated in shm_init.
 */
static char *device_buffer = NULL;

/*
 * Module initialization
 */
static int __init shm_init(void)
{
    int ret;

    printk(KERN_INFO "%sInitializing module\n", shm_prefix);

    // Allocate a device number
    ret = alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ALERT "%sFailed to allocate char dev region\n", shm_prefix);
        return ret;
    }
    major = MAJOR(dev_number);
    printk(KERN_INFO "%sRegistered with major number %d\n", shm_prefix, major);

    // Initialize the character device
    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;
    ret = cdev_add(&my_cdev, dev_number, 1);
    if (ret < 0) {
        unregister_chrdev_region(dev_number, 1);
        printk(KERN_ALERT "%sFailed to add cdev\n", shm_prefix);
        return ret;
    }

    // Create a device class
    mmap_class = class_create(CLASS_NAME);
    if (IS_ERR(mmap_class)) {
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_number, 1);
        printk(KERN_ALERT "%sFailed to create class\n", shm_prefix);
        return PTR_ERR(mmap_class);
    }

    // Create the device node in /dev
    if (device_create(mmap_class, NULL, dev_number, NULL, DEVICE_NAME) == NULL) {
        class_destroy(mmap_class);
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_number, 1);
        printk(KERN_ALERT "%sFailed to create device\n", shm_prefix);
        return -1;
    }

    // Allocate 4MB of memory for the device
    device_buffer = vmalloc(MEM_SIZE);
    if (!device_buffer) {
        device_destroy(mmap_class, dev_number);
        class_destroy(mmap_class);
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_number, 1);
        printk(KERN_ALERT "%sFailed to allocate memory\n", shm_prefix);
        return -ENOMEM;
    }
    memset(device_buffer, 0, MEM_SIZE);
    printk(KERN_INFO "%sAllocated 4MB memory at %p\n", shm_prefix, device_buffer);

    return 0;
}

/*
 * Module exit (cleanup)
 */
static void __exit shm_exit(void)
{
    if (device_buffer)
        vfree(device_buffer);
    device_destroy(mmap_class, dev_number);
    class_destroy(mmap_class);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_number, 1);
    printk(KERN_INFO "%sModule removed\n", shm_prefix);
}

module_init(shm_init);
module_exit(shm_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A char device driver with 4MB memory and mmap support");

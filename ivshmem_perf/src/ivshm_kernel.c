#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/pci.h>
#include <linux/uaccess.h>

#define IVSHMEM_BAR 0
#define IVSHMEM_VENDOR_ID 0x1AF4
#define IVSHMEM_DEVICE_ID 0x1110

#define DEVICE_NAME "ivshmem_secure"
#define IVSHMEM_SIZE (4 * 1024 * 1024)
#define CONTROL_SECTION_SIZE (64 * 1024)

struct IvshmemEntry {
  int sender_vm;
  int sender_pid;
  int receiver_vm;
  unsigned long mem_ptr; /* offset within the data section */
  unsigned int mem_size; /* allocated size in bytes */
  int ref_count;         /* usage count / if data is consumed */
};

struct IvshmemControl {
  /* Next free offset in the data section */
  unsigned long free_start_ptr;

  /* Number of active communication entries */
  unsigned int num_entry;

  /* A lock to protect control section changes (resizing, new allocations) */
  struct mutex resizing_lock;

  /* Array of IVSHMEM_Entry; we keep it simple here with a fixed size. */
  struct IVSHMEM_Entry entries[128];
};

static struct pci_dev *ivshmem_dev = NULL;
static void __iomem
    *ivshmem_base; /* Pointer to the entire shm region (kernel mapping) */
static struct cdev ivshmem_cdev;
static dev_t dev_num;
static struct class *ivshmem_class;

static int ivshmem_probe(struct pci_dev *pdev, const struct pci_device_id *id) {
  struct IvshmemControl *ctrl;

  if (pci_enable_device(pdev)) return -ENODEV;

  /* Map the PCI BAR (0) into kernel space */
  ivshmem_base = pci_iomap(pdev, IVSHMEM_BAR, IVSHMEM_SIZE);
  if (!ivshmem_base) {
    pr_err("Failed to map IVSHMEM BAR\n");
    return -ENOMEM;
  }

  /* Initialize the control section at offset 0 */
  ctrl = (struct IvshmemControl *)ivshmem_base;
  mutex_init(&ctrl->resizing_lock);
  ctrl->free_start_ptr = CONTROL_SECTION_SIZE;
  ctrl->num_entry = 0;
  memset(ctrl->entries, 0, sizeof(ctrl->entries));

  /* Save the device pointer for cleanup later */
  ivshmem_dev = pdev;

  /* Register character device */
  if (alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME) < 0) {
    pr_err("[ivshmem] Cannot allocate char device region\n");
    return -1;
  }

  cdev_init(&ivshmem_cdev, &ivshmem_fops);
  if (cdev_add(&ivshmem_cdev, dev_num, 1) < 0) {
    pr_err("[ivshmem] Cannot add cdev\n");
    unregister_chrdev_region(dev_num, 1);
    return -1;
  }

  /* Optional: create a device node in /dev using a class */
  ivshmem_class = class_create(THIS_MODULE, DEVICE_NAME);
  if (IS_ERR(ivshmem_class)) {
    pr_err("Failed to create class\n");
    cdev_del(&ivshmem_cdev);
    unregister_chrdev_region(dev_num, 1);
    return PTR_ERR(ivshmem_class);
  }
  device_create(ivshmem_class, NULL, dev_num, NULL, DEVICE_NAME);

  pr_info("IVSHMEM driver probed successfully. /dev/%s ready.\n", DEVICE_NAME);
  return 0;
}

static void ivshmem_remove(struct pci_dev *pdev) {
  device_destroy(ivshmem_class, dev_num);
  class_destroy(ivshmem_class);
  cdev_del(&ivshmem_cdev);
  unregister_chrdev_region(dev_num, 1);

  if (ivshmem_base) pci_iounmap(pdev, ivshmem_base);

  pr_info("IVSHMEM driver removed.\n");
}

static const struct pci_device_id ivshmem_ids[] = {
    {PCI_DEVICE(IVSHMEM_VENDOR_ID, IVSHMEM_DEVICE_ID)},
    {
        0,
    }};

static struct pci_driver ivshmem_driver = {
    .name = DEVICE_NAME,
    .id_table = ivshmem_ids,
    .probe = ivshmem_probe,
    .remove = ivshmem_remove,
};

static int __init ivshmem_init(void) {
  return pci_register_driver(&ivshmem_driver);
}

static void __exit ivshmem_exit(void) {
  pci_unregister_driver(&ivshmem_driver);
}

module_init(ivshmem_init);
module_exit(ivshmem_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Example Author");
MODULE_DESCRIPTION("Secure IVSHMEM Kernel Module Demo");

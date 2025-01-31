#ifndef __IVSHMEM_LIB_H__
#define __IVSHMEM_LIB_H__

#include <stdint.h>
#include <stdio.h>

#define IVSHMEM_RUNNING_OS 2 // 1: QEMU_HOST, 2: QEMU_VM

#if IVSHMEM_RUNNING_OS == 1 // QEMU_HOST
  #define IVSHMEM_RESOURCE0_PATH "/dev/shm/ivshmem"
  #define IVSHMEM_RESOURCE2_PATH "/dev/shm/ivshmem"
#elif IVSHMEM_RUNNING_OS == 2 // QEMU_VM
  #define IVSHMEM_RESOURCE0_PATH "/sys/class/pci_bus/0000:00/device/0000:00:04.0/resource0"
  #define IVSHMEM_RESOURCE2_PATH "/sys/class/pci_bus/0000:00/device/0000:00:04.0/resource2"
#endif


#define IVSHMEM_BAR0_SIZE 256
// #define IVSHMEM_PATH "~/ivshmem"
#define IVSHMEM_
#define IVSHMEM_SIZE (4 * 1024 * 1024)
#define CONTROL_SECTION_SIZE (64 * 1024)

struct IvshmemDeviceContext {
  int bar0_fd;
  uint32_t *p_reg;

  int bar2_fd;
  uint32_t *p_shmem;

  size_t shmem_size;
};

struct IvshmemChannelKey {
  int sender_vm;
  int sender_pid;
  int receiver_vm;
};

struct IvshmemChannel {
  struct IvshmemChannelKey key;
  size_t buf_offset; /* offset within the data section */
  size_t buf_size;   /* allocated size in bytes */
  size_t data_size;  /* actual data size in bytes */
  int ref_count;     /* usage count / if data is consumed */
};

struct IvshmemControlSection {
  /* Next free offset in the data section */
  size_t free_start_offset;

  /* Number of active communication entries */
  unsigned int num_entry;

  // /* A lock to protect control section changes (resizing, new allocations) */
  // struct mutex resizing_lock;

  struct IvshmemChannel channels[128];
};

int ivshmem_open_fd();

struct IvshmemChannel *ivshmem_read_channel(
    struct IvshmemControlSection *ctr_sec, struct IvshmemChannelKey *key);

void ivshmem_init_dev_ctx(struct IvshmemDeviceContext *p_dev_ctx);

uint32_t _ivshmem_get_shmem_size();

int ivshmem_open_dev(struct IvshmemDeviceContext *p_dev_ctx);

void ivshmem_close_dev(struct IvshmemDeviceContext *p_dev_ctx);


#endif

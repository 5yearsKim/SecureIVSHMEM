#ifndef __IVSHMEM_LIB_H__
#define __IVSHMEM_LIB_H__

#include <stdio.h>

// #define IVSHMEM_PATH "/sys/bus/pci/devices/0000:00:04.0/resource2"
#define IVSHMEM_PATH "~/ivshmem"
#define IVSHMEM_SIZE (4 * 1024 * 1024)
#define CONTROL_SECTION_SIZE (64 * 1024)

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

#endif

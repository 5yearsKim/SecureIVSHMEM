#ifndef __IVSHMEM_DATA_H__
#define __IVSHMEM_DATA_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define IVSHMEM_MAX_CHANNELS 128

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

  /* Number of active communication channel */
  unsigned int num_active_channels;

  // /* A lock to protect control section changes (resizing, new allocations) */
  // struct mutex resizing_lock;

  struct IvshmemChannel channels[IVSHMEM_MAX_CHANNELS];

  // /* A lock to protect controls/changes channels */
  int control_mutex;
  bool has_channel_candidate;
  struct IvshmemChannelKey channel_candidate;
};

struct IvshmemChannel *ivshmem_read_channel(
    struct IvshmemControlSection *ctr_sec, struct IvshmemChannelKey *key);

#endif
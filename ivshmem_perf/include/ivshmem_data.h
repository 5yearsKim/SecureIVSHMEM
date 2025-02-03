#ifndef __IVSHMEM_DATA_H__
#define __IVSHMEM_DATA_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>

#define IVSHMEM_MAX_CHANNELS 32
#define DATA_SECTION_SIZE (IVSHMEM_SIZE - sizeof(struct IvshmemControlSection))

struct IvshmemChannelKey {
  int sender_vm;
  int sender_pid;
  int receiver_vm;
};

struct IvshmemChannel {
  unsigned int id;
  struct IvshmemChannelKey key;
  size_t buf_offset;   /* offset within the data section */
  size_t buf_size;     /* allocated size in bytes */
  size_t data_size;    /* actual data size in bytes */
  int ref_count;       /* usage count / if data is consumed */
  int sent_count;      /* number of sent messages in rebalancing interval */
  time_t last_sent_at; /* last sent time */
};

struct IvshmemControlSection {
  /* Next free offset in the data section */
  size_t free_start_offset;

  /* Number of active communication channel */
  unsigned int num_active_channels;

  /* Used for assigning id for channel */
  unsigned int last_channel_id;

  struct IvshmemChannel channels[IVSHMEM_MAX_CHANNELS];

  // /* A lock to protect controls/changes channels */
  int control_mutex;
  bool has_channel_candidate;
  struct IvshmemChannelKey channel_candidate;
};

void *ivshmem_get_data_section(struct IvshmemControlSection *p_ctr_sec);

struct IvshmemChannel *ivshmem_find_channel(
    struct IvshmemControlSection *p_ctr_sec, struct IvshmemChannelKey *key);

struct IvshmemChannel *ivshmem_request_channel(
    struct IvshmemControlSection *p_ctr_sec, struct IvshmemChannelKey *key);



#endif
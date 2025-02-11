#ifndef __IVSHMEM_SECURE_H__
#define __IVSHMEM_SECURE_H__

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>

#define IVSHMEM_MAX_CHANNELS 32
#define DATA_SECTION_SIZE (IVSHMEM_SIZE - sizeof(struct IvshmemControlSection))

#define IVSHMEM_TYPE 2  // 1: chunk, 2: page
#define IVSHMEM_PAGE_SIZE (4 * 1024)

struct IvshmemChannelKey {
  uint32_t sender_vm;
  uint32_t sender_pid;
  uint32_t receiver_vm;
};

struct IvshmemChannel {
  unsigned int id;
  struct IvshmemChannelKey key;
  size_t buf_offset;            /* offset within the data section */
  size_t buf_size;              /* allocated size in bytes */
  _Atomic size_t data_size;     /* actual data size in bytes */
  _Atomic uint8_t sender_itr;   /* sender interrupt */
  _Atomic uint8_t receiver_itr; /* receiver interrupt */

#if IVSHMEM_TYPE == 1
  int ref_count;  /* usage count / if data is consumed */
  int sent_count; /* number of sent messages in rebalancing interval */
#elif IVSHMEM_TYPE == 2
  uint32_t tail;  /* tail idx */
  uint32_t head;  /* head idx */
  int num_page;   /* max num page */
  int head_touch; /* number of head touch */
#else
#error "Invalid IVSHMEM_TYPE"
#endif
  time_t last_sent_at; /* last sent time */
};

struct IvshmemControlSection {
  /* Next free offset in the data section */
  size_t free_start_offset;

  /* Number of active communication channel */
  int num_active_channels;

  /* Used for assigning id for channel */
  unsigned int last_channel_id;

  struct IvshmemChannel channels[IVSHMEM_MAX_CHANNELS];

  // /* A lock to protect controls/changes channels */
  _Atomic uint8_t control_mutex;
  bool has_channel_candidate;
  struct IvshmemChannelKey channel_candidate;
};

void *ivshmem_get_data_section(struct IvshmemControlSection *p_ctr_sec);

struct IvshmemChannel *ivshmem_find_channel(
    struct IvshmemControlSection *p_ctr_sec, struct IvshmemChannelKey *key);

struct IvshmemChannel *ivshmem_request_channel(
    struct IvshmemControlSection *p_ctr_sec, struct IvshmemChannelKey *key);

int ivshmem_send_buffer(struct IvshmemChannelKey *p_key,
                        struct IvshmemControlSection *p_ctr, void *p_buffer,
                        size_t size);

int ivshmem_recv_buffer(struct IvshmemChannelKey *p_key,
                        struct IvshmemControlSection *p_ctr, void *p_buffer,
                        size_t size);

#endif  // __IVSHMEM_SECURE_H__
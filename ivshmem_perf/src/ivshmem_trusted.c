#include "ivshmem_trusted.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* Initialize a control section with default values */
int ivshmem_init_control_section(struct IvshmemControlSection *p_ctr_sec,
                                 void *p_data) {
  p_ctr_sec->free_start_offset = 0;
  p_ctr_sec->num_active_channels = 0;
  p_ctr_sec->last_channel_id = 1;
  p_ctr_sec->control_mutex = 0;
  p_ctr_sec->has_channel_candidate = false;

  memset(p_ctr_sec->channels, 0,
         sizeof(struct IvshmemChannel) * IVSHMEM_MAX_CHANNELS);
  memset(p_data, 0, DATA_SECTION_SIZE);
  return 0;
}

/* Check if a control (channel creation) request has been issued */
int ivshmem_check_is_control_requested(
    struct IvshmemControlSection *p_ctr_sec) {
  return (p_ctr_sec->has_channel_candidate) ? 1 : 0;
}

int ivshmem_trusted_rebalancing(struct IvshmemControlSection *p_ctr_sec,
                                void *p_data) {
  if (p_ctr_sec->num_active_channels <= 0) {
    return 0;
  }

  p_ctr_sec->control_mutex = 1;  // Lock the control section

  time_t now = time(NULL);


  size_t allocated_buffer_size =
      (DATA_SECTION_SIZE - IVSHMEM_CHANNEL_INIT_SIZE - 1) /
      p_ctr_sec->num_active_channels;

#if IVSHMEM_TYPE == 2
  // allocated_buffer_size should be multiple of IVSHMEM_PAGE_SIZE
  int num_page = allocated_buffer_size / IVSHMEM_PAGE_SIZE;
  allocated_buffer_size = num_page * IVSHMEM_PAGE_SIZE;
#endif


  if (allocated_buffer_size == p_ctr_sec->channels[0].buf_size) {
    // no change in buffer size
    printf("No change in buffer size, skipping rebalancing..\n");
    p_ctr_sec->control_mutex = 0;  // Unlock the control section
    return 0;
  }

  struct IvshmemChannel *p_chan_tmp =
      malloc(sizeof(struct IvshmemChannel) * p_ctr_sec->num_active_channels);
  if (p_chan_tmp == NULL) {
    printf("Failed to allocate memory for temporary channel array\n");
    return -1;
  }
  memcpy(p_chan_tmp, p_ctr_sec->channels,
         sizeof(struct IvshmemChannel) * p_ctr_sec->num_active_channels);

  p_ctr_sec->free_start_offset = 0;



  int cnt = 0;
  for (unsigned int i = 0; i < p_ctr_sec->num_active_channels; i++) {
    struct IvshmemChannel *p_chan = &p_chan_tmp[i];

    p_ctr_sec->channels[cnt] = *p_chan;
    p_ctr_sec->channels[cnt].buf_offset = p_ctr_sec->free_start_offset;
    p_ctr_sec->channels[cnt].buf_size = allocated_buffer_size;
    printf("alllocating buffer size %zu to index %d\n", p_ctr_sec->channels[cnt].buf_size, cnt);
    p_ctr_sec->free_start_offset += allocated_buffer_size;

#if IVSHMEM_TYPE == 1
    p_ctr_sec->channels[cnt].sent_count = 0;
#elif IVSHMEM_TYPE == 2

    p_ctr_sec->channels[cnt].tail = p_chan->tail % num_page;
    p_ctr_sec->channels[cnt].head = p_chan->head % num_page;
    p_ctr_sec->channels[cnt].num_page = num_page;
    p_ctr_sec->channels[cnt].head_touch = 0;
#else
#error "Invalid IVSHMEM_TYPE"
#endif

    cnt++;
  }
  p_ctr_sec->num_active_channels = cnt;
  memset(&p_ctr_sec->channels[cnt], 0,
         sizeof(struct IvshmemChannel) * (IVSHMEM_MAX_CHANNELS - cnt));

  free(p_chan_tmp);              // Free the temporary channel array
  p_ctr_sec->control_mutex = 0;  // Unlock the control section

  return 0;
}

/*
 * Process pending control requests:
 * If a channel candidate exists, check available space and create a new
 * channel.
 */
int ivshmem_trusted_control(struct IvshmemControlSection *p_ctr_sec) {
  if (!p_ctr_sec->has_channel_candidate) {
    // No pending request.
    return 0;
  }

  p_ctr_sec->control_mutex = 1;  // Lock the control section

  /* Create the new channel */
  int ret = ivshmem_create_channel(p_ctr_sec, &p_ctr_sec->channel_candidate);
  if (ret == 0) {
    printf("Channel created: sender_vm=%d, receiver_vm=%d\n",
           p_ctr_sec->channel_candidate.sender_vm,
           p_ctr_sec->channel_candidate.receiver_vm);
  } else {
    printf("Failed to create channel\n");
  }
  p_ctr_sec->has_channel_candidate = false;

  p_ctr_sec->control_mutex = 0;  // Unlock the control section
  return ret;
}

/*
 * Create a new channel and update the control section.
 * Returns 0 on success, -1 on error.
 */
int ivshmem_create_channel(struct IvshmemControlSection *p_ctr_sec,
                           struct IvshmemChannelKey *p_key) {
  if (p_ctr_sec->num_active_channels >= IVSHMEM_MAX_CHANNELS) {
    printf("No more channels can be created\n");
    return -1;
  }
  if (p_ctr_sec->free_start_offset + IVSHMEM_CHANNEL_INIT_SIZE >
      DATA_SECTION_SIZE) {
    printf("No more space for channel\n");
    return -1;
  }

  /* Create and initialize the new channel in the next available slot */
  struct IvshmemChannel *new_channel =
      &p_ctr_sec->channels[p_ctr_sec->num_active_channels];

  ivshmem_init_channel(p_ctr_sec, new_channel, p_key);

  /* Update control section */
  p_ctr_sec->last_channel_id++;
  p_ctr_sec->free_start_offset += IVSHMEM_CHANNEL_INIT_SIZE;
  p_ctr_sec->num_active_channels++;

  return 0;
}

/*
 * Find an existing channel with the given key; if not found, create one.
 */
struct IvshmemChannel *ivshmem_find_or_create_channel(
    struct IvshmemControlSection *p_ctr_sec, struct IvshmemChannelKey *key) {
  struct IvshmemChannel *p_channel = ivshmem_find_channel(p_ctr_sec, key);
  if (p_channel) return p_channel;

  if (ivshmem_create_channel(p_ctr_sec, key) != 0) {
    return NULL;
  }


  /* Return the newly created channel (the last in the channels array) */
  return &p_ctr_sec->channels[p_ctr_sec->num_active_channels - 1];
}

/*
 * Initialize a channel with the provided key.
 */
void ivshmem_init_channel(struct IvshmemControlSection *p_ctr_sec,
                          struct IvshmemChannel *p_channel,
                          struct IvshmemChannelKey *key) {
  p_channel->id = p_ctr_sec->last_channel_id;
  p_channel->key.sender_vm = key->sender_vm;
  p_channel->key.sender_pid = key->sender_pid;
  p_channel->key.receiver_vm = key->receiver_vm;

  p_channel->buf_offset = p_ctr_sec->free_start_offset;
  p_channel->buf_size = IVSHMEM_CHANNEL_INIT_SIZE;
  p_channel->data_size = 0;
  p_channel->last_sent_at = time(NULL);

#if IVSHMEM_TYPE == 1
  p_channel->ref_count = 0;
  p_channel->sent_count = 0;
#elif IVSHMEM_TYPE == 2
  p_channel->tail = 0;
  p_channel->head = 0;
  p_channel->num_page = IVSHMEM_CHANNEL_INIT_SIZE / IVSHMEM_PAGE_SIZE;
  p_channel->head_touch = 0;
#else
#error "Invalid IVSHMEM_TYPE"
#endif
  p_channel->last_sent_at = 0;
}

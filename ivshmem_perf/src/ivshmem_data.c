#include "ivshmem_data.h"

#include <unistd.h>
#include <string.h>
#include <time.h>
#define MIN(a, b) ((a) < (b) ? (a) : (b))

void *ivshmem_get_data_section(struct IvshmemControlSection *p_ctr_sec) {
  return (void *)p_ctr_sec + sizeof(struct IvshmemControlSection);
}

struct IvshmemChannel *ivshmem_find_channel(
    struct IvshmemControlSection *p_ctr_sec, struct IvshmemChannelKey *key) {
  // Find the channel with the same key
  for (int i = 0; i < p_ctr_sec->num_active_channels; i++) {
    if (p_ctr_sec->channels[i].key.sender_vm == key->sender_vm &&
        (key->sender_pid < 0 ||
         p_ctr_sec->channels[i].key.sender_pid == key->sender_pid) &&
        p_ctr_sec->channels[i].key.receiver_vm == key->receiver_vm) {
      return &p_ctr_sec->channels[i];
    }
  }
  return NULL;
}

struct IvshmemChannel *ivshmem_request_channel(
    struct IvshmemControlSection *p_ctr_sec, struct IvshmemChannelKey *key) {
  struct IvshmemChannel *p_channel;
  p_channel = ivshmem_find_channel(p_ctr_sec, key);

  if (p_channel) {
    return p_channel;
  }

  p_ctr_sec->has_channel_candidate = true;
  p_ctr_sec->channel_candidate = *key;

  // wait for the channel to be created
  printf("DEBUG: waiting for channel to be created\n");
  while (p_ctr_sec->has_channel_candidate) {
    usleep(10 * 1000);
  }
  p_channel = ivshmem_find_channel(p_ctr_sec, key);

  return p_channel;
}

int ivshmem_send_buffer(struct IvshmemChannel *p_chan, void *p_shm,
                        void *p_buffer, size_t size) {
  size_t data_sent = 0;

  while (1) {
    if (data_sent >= size) {
      break;
    }
    if (p_chan->ref_count > 0) {
      continue;
    }
    int to_sent = MIN(size - data_sent, p_chan->buf_size);

    memcpy((void *)((char *)p_shm + p_chan->buf_offset), p_buffer, to_sent);
    data_sent += to_sent;

    // update channel
    p_chan->data_size = to_sent;
    p_chan->ref_count = 1;
    p_chan->last_sent_at = time(NULL);
  }

  return 0;
}

int ivshmem_recv_buffer(struct IvshmemChannel *channel, void *p_shm,
                           void *buffer, size_t size) {
  int data_received = 0;

  while (1) {
    if (channel->ref_count == 0) {
      continue;
    }

    if (data_received >= size) {
      break;
    }
    int to_receive = MIN(size - data_received, channel->data_size);

    memcpy(buffer, (void *)((char *)p_shm + channel->buf_offset), to_receive);
    data_received += to_receive;

    // update channel
    channel->data_size = 0;
    channel->ref_count-- ;
  }

  return 0;
}
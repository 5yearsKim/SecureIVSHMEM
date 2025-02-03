#include "ivshmem_data.h"

#include <unistd.h>

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


int ivshmem_send_data(struct IvshmemChannel *channel, void *data, size_t size) {
  if (channel->buf_size - channel->data_size < size) {
    printf("No more space in the channel\n");
    return -1;
  }

  memcpy((void *)((char *)channel + channel->buf_offset + channel->data_size),
         data, size);
  channel->data_size += size;
  return 0;
}


int ivshmem_receive_data(struct IvshmemChannel *channel, void *data, size_t size) {
  if (channel->data_size < size) {
    printf("No more data in the channel\n");
    return -1;
  }

  memcpy(data, (void *)((char *)channel + channel->buf_offset), size);
  channel->data_size -= size;
  return 0;
}
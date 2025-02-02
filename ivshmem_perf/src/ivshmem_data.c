#include "ivshmem_data.h"

struct IvshmemChannel *ivshmem_find_or_create_channel(
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
  // Create a new channel if not found
  if (p_ctr_sec->num_active_channels >= IVSHMEM_MAX_CHANNELS) {
    printf("No more channel can be created\n");
    return NULL;
  }

  struct IvshmemChannel *new_channel =
      &p_ctr_sec->channels[p_ctr_sec->num_active_channels];
  new_channel->key.sender_vm = key->sender_vm;
  new_channel->key.sender_pid = key->sender_pid;
  new_channel->key.receiver_vm = key->receiver_vm;
  new_channel->buf_offset = p_ctr_sec->free_start_offset;
  new_channel->buf_size = 0;
  new_channel->data_size = 0;
  new_channel->ref_count = 0;

  return NULL;
}


void ivshmem_init_channel(struct IvshmemChannel *p_channel,
                          struct IvshmemChannelKey *key) {
  p_channel->key.sender_vm = key->sender_vm;
  p_channel->key.sender_pid = key->sender_pid;
  p_channel->key.receiver_vm = key->receiver_vm;
  p_channel->buf_offset = 0;
  p_channel->buf_size = 0;
  p_channel->data_size = 0;
  p_channel->ref_count = 0;
}
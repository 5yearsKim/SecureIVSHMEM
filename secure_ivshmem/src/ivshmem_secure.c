#include "ivshmem_secure.h"

#include <string.h>
#include <time.h>
#include <unistd.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

void *ivshmem_get_data_section(struct IvshmemControlSection *p_ctr_sec) {
  return (void *)((char *)p_ctr_sec + sizeof(struct IvshmemControlSection));
}

struct IvshmemChannel *ivshmem_find_channel(
    struct IvshmemControlSection *p_ctr_sec, struct IvshmemChannelKey *key) {
  // Find the channel with the same key
  for (int i = 0; i < p_ctr_sec->num_active_channels; i++) {
    if (p_ctr_sec->channels[i].key.sender_vm == key->sender_vm &&
        (key->sender_pid == 0 ||
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

int ivshmem_send_buffer(struct IvshmemChannelKey *p_key,
                        struct IvshmemControlSection *p_ctr, void *p_buffer,
                        size_t size) {
  struct IvshmemChannel *p_chan = ivshmem_request_channel(p_ctr, p_key);
  if (p_chan == NULL) {
    fprintf(stderr, "Failed to request channel\n");
    return -1;
  }

  void *p_data = ivshmem_get_data_section(p_ctr);

  size_t data_sent = 0;

  while (1) {
    if (p_ctr->control_mutex != 0) {
      continue;  // wait for the control section to be free
    }

    if (data_sent >= size) {
      break;
    }
#if IVSHMEM_TYPE == 1

    if (p_chan->ref_count > 0) {
      continue;
    }
    size_t to_sent = MIN(size - data_sent, p_chan->buf_size);

    memcpy((char *)p_data + p_chan->buf_offset, (char *)p_buffer + data_sent,
           to_sent);

    p_chan->ref_count = 1;

#elif IVSHMEM_TYPE == 2
    // check if the channel is full
    if (p_chan->head == p_chan->tail - 1 ||
        (p_chan->head == p_chan->num_page - 1 && p_chan->tail == 0)) {
      usleep(1 * 10);
      continue;
    }

    size_t to_sent = MIN(size - IVSHMEM_PAGE_SIZE, IVSHMEM_PAGE_SIZE);

    memcpy((char *)p_data + p_chan->buf_offset +
               (p_chan->head * IVSHMEM_PAGE_SIZE),
           (char *)p_buffer + data_sent, to_sent);

    p_chan->head = (p_chan->head + 1) % p_chan->num_page;

#else
#error "Invalid IVSHMEM_TYPE"
#endif

    data_sent += to_sent;

    atomic_fetch_add(&p_chan->data_size, to_sent);

    p_chan->last_sent_at = time(NULL);
  }

  return 0;
}

int ivshmem_recv_buffer(struct IvshmemChannelKey *p_key,
                        struct IvshmemControlSection *p_ctr, void *p_buffer,
                        size_t size) {
  struct IvshmemChannel *p_chan = ivshmem_find_channel(p_ctr, p_key);
  if (p_chan == NULL) {
    printf(" DEBUG: Channel not found\n");
    return -1;
  }

  void *p_data = ivshmem_get_data_section(p_ctr);

  int data_received = 0;

  while (1) {
    if (p_ctr->control_mutex != 0) {
      continue;  // wait for the control section to be free
    }

    if (data_received >= size) {
      break;
    }

#if IVSHMEM_TYPE == 1

    if (p_chan->ref_count == 0) {
      continue;
    }

    int to_receive = MIN(size - data_received, p_chan->data_size);
    memcpy((char *)p_buffer + data_received,
           ((char *)p_data + p_chan->buf_offset), to_receive);
    data_received += to_receive;

    // update channel
    atomic_store(&p_chan->data_size, 0);
    p_chan->ref_count--;

#elif IVSHMEM_TYPE == 2

    if (p_chan->head == p_chan->tail) {
      // printf("DEBUG: channel is empty\n");
      continue;
    }

    int to_receive = MIN(size - data_received, IVSHMEM_PAGE_SIZE);

    memcpy((char *)p_buffer + data_received,
           ((char *)p_data + p_chan->buf_offset +
            (p_chan->tail * IVSHMEM_PAGE_SIZE)),
           to_receive);

    p_chan->tail = (p_chan->tail + 1) % p_chan->num_page;

    data_received += to_receive;

    atomic_fetch_sub(&p_chan->data_size, to_receive);

#else
#error "Invalid IVSHMEM_TYPE"
#endif
  }

  return 0;
}
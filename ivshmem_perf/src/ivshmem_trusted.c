#include "ivshmem_trusted.h"

#define IVSHMEM_CHANNEL_INIT_SIZE 100 * 1024

int ivshmem_check_is_control_requested(struct IvshmemControlSection* p_ctr_sec) {
  if (p_ctr_sec->has_channel_candidate) {
    return 1;
  }
  return 0;
}

void ivshmem_trusted_control(struct IvshmemControlSection* p_ctr_sec) {
  if (p_ctr_sec->has_channel_candidate) {
    // if channel candidate is set, create a new channel
  }
  // if () // if 
}

int ivshmem_create_channel(struct IvshmemControlSection* p_ctr_sec, struct IvshmemChannelKey* p_key) {
  // check if free space is available
  if (p_ctr_sec->num_active_channels >= IVSHMEM_MAX_CHANNELS) {
    printf("No more channel can be created\n");
    return -1;
  }
  if (p_ctr_sec->free_start_offset + IVSHMEM_CHANNEL_INIT_SIZE > DATA_SECTION_SIZE) {
    printf("No more space for channel\n");
    return -1;
  }

  // create a new channel
  // TODO

}


int ivshmem_rebalnce_channel(struct IvshmemControlSection* p_ctr_sec) {
  // rebalance the channel
  // TODO
}
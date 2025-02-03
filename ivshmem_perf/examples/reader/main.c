#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  // Include for memcpy
#include <sys/mman.h>
#include <unistd.h>

#include "ivshmem_data.h"
#include "ivshmem_lib.h"

int main(int argc, char **argv) {
  uint32_t *p_data;
  struct IvshmemDeviceContext dev_ctx;
  struct IvshmemControlSection *p_ctr;
  struct IvshmemChannel prev_channel, *channel = NULL;
  int fd;
  int i;

  printf("Initializing device context...\n");
  ivshmem_init_dev_ctx(&dev_ctx);
  printf("Opening device...\n");
  ivshmem_open_dev(&dev_ctx);

  printf("size of IvshmemControlSection: %lu\n", sizeof(struct IvshmemControlSection));
  printf("size of data section: %lu\n", DATA_SECTION_SIZE);

  p_ctr = (struct IvshmemControlSection *)dev_ctx.p_shmem;
  p_data = ivshmem_get_data_section(p_ctr);

  struct IvshmemChannelKey key = {
      .sender_vm = 1, .sender_pid = -1, .receiver_vm = 2};

  channel= ivshmem_find_channel(p_ctr, &key);

  if (!channel) {
    printf("Channel not found :(\n");
    return -1;
  }
  printf("Channel found! Buffer Size: %zu, Data Size: %zu\n",
        channel->buf_size, channel->data_size);

  prev_channel = *channel;



  while (1) {

    if (prev_channel.id != channel->id) {

      printf("## channel id changed, finding again.. \n");
      struct IvshmemChannel *channel= ivshmem_find_channel(p_ctr, &key);

      if (!channel) {
        printf("Channel not found :(\n");
        return -1;
      }
    }

    printf("## channel info ##\n");
    printf("id: %d\n", channel->id);
    printf("buf_offset: %zu\n", channel->buf_offset);
    printf("buf_size: %zu\n", channel->buf_size);
    printf("data_size: %zu\n", channel->data_size);
    printf("ref_count: %d\n", channel->ref_count);
    printf("sent_count: %d\n", channel->sent_count);
    printf("last_sent_at: %ld\n", channel->last_sent_at);

    prev_channel = *channel;


    sleep(1);
  }


cleanup:

  ivshmem_close_dev(&dev_ctx);

  return 0;
}

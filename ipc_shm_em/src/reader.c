#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  // Include for memcpy
#include <sys/mman.h>
#include <unistd.h>

#include "ivshmem_lib.h"

int main(int argc, char **argv) {
  uint32_t *p_data;
  struct IvshmemDeviceContext dev_ctx;
  struct IvshmemControlSection *p_ctr;
  int fd;
  int i;

  ivshmem_init_dev_ctx(&dev_ctx);
  ivshmem_open_dev(&dev_ctx);

  p_ctr = (struct IvshmemControlSection*) dev_ctx.p_shmem;
  p_data = dev_ctx.p_shmem + CONTROL_SECTION_SIZE;

  struct IvshmemChannelKey key = {
      .sender_vm = 1, .sender_pid = -1, .receiver_vm = 2};
  struct IvshmemChannel *channel = ivshmem_read_channel(p_ctr, &key);

  if (channel) {
    printf("Channel found! Buffer Size: %zu, Data Size: %zu\n",
           channel->buf_size, channel->data_size);
  } else {
    printf("Channel not found!\n");
  }

  ivshmem_close_dev(&dev_ctx);

  return 0;
}

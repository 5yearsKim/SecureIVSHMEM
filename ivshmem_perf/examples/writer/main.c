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
  int fd;
  int i, ret;

  const char *data_to_write = "this is a sample data";

  printf("init from here\n");

  // fd = open(IVSHMEM_PATH, O_RDWR);
  // printf("hello, fd: %d\n", fd);
  // assert(fd != -1);

  ivshmem_init_dev_ctx(&dev_ctx);
  ret = ivshmem_open_dev(&dev_ctx);

  assert(ret == 0);

  p_ctr = (struct IvshmemControlSection *)dev_ctx.p_shmem;
  p_data = ivshmem_get_data_section(p_ctr);


  struct IvshmemChannelKey key = {
      .sender_vm = 1, .sender_pid = 1234, .receiver_vm = 2};

  printf("Requesting channel...\n");
  struct IvshmemChannel *channel = ivshmem_request_channel(p_ctr, &key);

  if (!channel) {
    printf("Failed to request a channel :(\n");
    return -1;
  }

  

  printf("Channel requested! Buffer Size: %zu, Data Size: %zu\n",
          channel->buf_size, channel->data_size);


  ivshmem_close_dev(&dev_ctx);
  close(fd);

  return 0;
}

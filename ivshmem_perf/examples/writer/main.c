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
  p_data = dev_ctx.p_shmem + CONTROL_SECTION_SIZE;

  printf("Control section:\n");

  // initialize the control section
  p_ctr->free_start_offset = CONTROL_SECTION_SIZE;
  p_ctr->num_active_channels = 1;
  p_ctr->channels[0].key.sender_vm = 1;
  p_ctr->channels[0].key.sender_pid = getpid();
  p_ctr->channels[0].key.receiver_vm = 2;
  p_ctr->channels[0].buf_offset = CONTROL_SECTION_SIZE;
  p_ctr->channels[0].buf_size = 512;
  p_ctr->channels[0].data_size = 25;
  p_ctr->channels[0].ref_count = 1;

  // Add buffer size to free_start_offset
  p_ctr->free_start_offset += 1024;

  ivshmem_close_dev(&dev_ctx);
  close(fd);

  return 0;
}

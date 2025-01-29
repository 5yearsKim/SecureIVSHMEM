#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  // Include for memcpy
#include <sys/mman.h>
#include <unistd.h>

#include "ivshmem_lib.h"

int main(int argc, char **argv) {
  void *p, *data_ptr;
  struct IvshmemControlSection *ctr_ptr;
  int fd;
  int i;

  const char *data_to_write = "this is a sample data";

  printf("init from here\n");

  // fd = open(IVSHMEM_PATH, O_RDWR);
  // printf("hello, fd: %d\n", fd);
  // assert(fd != -1);

  fd = ivshmem_open_fd();

  p = mmap(NULL, IVSHMEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (p == MAP_FAILED) {
    printf("Failed to map shared memory\n");
    close(fd);
    return EXIT_FAILURE;
  }

  printf("pointer location: %p\n", (void *)p);

  ctr_ptr = p;
  data_ptr = p + CONTROL_SECTION_SIZE;

  struct IvshmemChannelKey key = {
      .sender_vm = 1, .sender_pid = -1, .receiver_vm = 2};
  struct IvshmemChannel *channel = ivshmem_read_channel(ctr_ptr, &key);

  if (channel) {
    printf("Channel found! Buffer Size: %zu, Data Size: %zu\n",
           channel->buf_size, channel->data_size);
  } else {
    printf("Channel not found!\n");
  }

  // memcpy(data_ptr, data_to_write, strlen(data_to_write) + 1);

  // printf("data written: %s\n", data_ptr);

  // printf("Data written:\n");
  // for (i = 0; i < 100; i++) {
  //     printf("char %d: %c\n", i, data_ptr[i]);
  // }

  munmap(p, IVSHMEM_SIZE);
  close(fd);

  return 0;
}

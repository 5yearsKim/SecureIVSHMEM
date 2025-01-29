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

  printf("Control section:\n");

  // initialize the control section
  ctr_ptr->free_start_offset = CONTROL_SECTION_SIZE;
  ctr_ptr->num_entry = 1;
  ctr_ptr->channels[0].key.sender_vm = 1;
  ctr_ptr->channels[0].key.sender_pid = getpid();
  ctr_ptr->channels[0].key.receiver_vm = 2;
  ctr_ptr->channels[0].buf_offset = CONTROL_SECTION_SIZE;
  ctr_ptr->channels[0].buf_size = 1024;
  ctr_ptr->channels[0].data_size = 12;
  ctr_ptr->channels[0].ref_count = 1;

  // Add buffer size to free_start_offset
  ctr_ptr->free_start_offset += 1024;

  munmap(p, IVSHMEM_SIZE);
  close(fd);

  return 0;
}

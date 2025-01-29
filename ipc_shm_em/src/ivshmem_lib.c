
#include "ivshmem_lib.h"

#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

int ivshmem_open_fd() {
  int fd;
  const char *shmName = "/mysharedmem";

  fd = shm_open(shmName, O_CREAT | O_RDWR, 0666);
  if (fd == -1) {
    printf("Failed to create/open shared memory\n");
    return -1;
  }

  // 2. Set the size of the shared memory object
  if (ftruncate(fd, IVSHMEM_SIZE) == -1) {
    printf("Failed to set size for shared memory\n");
    return -1;
  }
  return fd;
}

struct IvshmemChannel *ivshmem_read_channel(
    struct IvshmemControlSection *ctr_sec, struct IvshmemChannelKey *key) {
  for (int i = 0; i < 128; i++) {
    if (ctr_sec->channels[i].key.sender_vm == key->sender_vm &&
        (key->sender_pid < 0 ||
         ctr_sec->channels[i].key.sender_pid == key->sender_pid) &&
        ctr_sec->channels[i].key.receiver_vm == key->receiver_vm) {
      return &ctr_sec->channels[i];
    }
  }
  return NULL;
}

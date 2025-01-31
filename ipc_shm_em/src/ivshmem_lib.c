
#include "ivshmem_lib.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
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
    struct IvshmemControlSection *p_ctr_sec, struct IvshmemChannelKey *key) {
  for (int i = 0; i < 128; i++) {
    if (p_ctr_sec->channels[i].key.sender_vm == key->sender_vm &&
        (key->sender_pid < 0 ||
         p_ctr_sec->channels[i].key.sender_pid == key->sender_pid) &&
        p_ctr_sec->channels[i].key.receiver_vm == key->receiver_vm) {
      return &p_ctr_sec->channels[i];
    }
  }
  return NULL;
}

// Functions from ACRN project

void ivshmem_init_dev_ctx(struct IvshmemDeviceContext *p_dev_ctx) {
  memset(p_dev_ctx, 0, sizeof(struct IvshmemDeviceContext));

  p_dev_ctx->bar0_fd = -1;
  p_dev_ctx->p_reg = NULL;
  p_dev_ctx->bar2_fd = -1;
  p_dev_ctx->p_shmem = NULL;
  p_dev_ctx->shmem_size = 0;
}

uint32_t _ivshmem_get_shmem_size() {
  uint64_t shm_size;
  uint64_t tmp;
  int cfg_fd;

  cfg_fd = open(IVSHMEM_RESOURCE0_PATH, O_RDWR);
  pread(cfg_fd, &tmp, 8, 0x18);

  shm_size = ~0U;
  pwrite(cfg_fd, &shm_size, 8, 0x18);
  pread(cfg_fd, &shm_size, 8, 0x18);
  pwrite(cfg_fd, &tmp, 8, 0x18);
  shm_size &= (~0xfUL);
  shm_size = (shm_size & ~(shm_size - 1));
  close(cfg_fd);

  return shm_size;
}

int ivshmem_open_dev(struct IvshmemDeviceContext *p_dev_ctx) {

  p_dev_ctx->bar0_fd = open(IVSHMEM_RESOURCE0_PATH, O_RDWR);
  if (p_dev_ctx->bar0_fd < 0) {
    printf("Failed to open %s\n", IVSHMEM_RESOURCE0_PATH);
    return -1;
  }

  p_dev_ctx->p_reg = mmap(NULL, IVSHMEM_BAR0_SIZE, PROT_READ | PROT_WRITE,
                          MAP_SHARED, p_dev_ctx->bar0_fd, 0);
  if (p_dev_ctx->p_reg == MAP_FAILED) {
    printf("Failed to map %s\n", IVSHMEM_RESOURCE0_PATH);
    close(p_dev_ctx->bar0_fd);
    return -1;
  }

  printf("IVSHMEM_RESOURCE2_PATH %s\n", IVSHMEM_RESOURCE2_PATH);

  p_dev_ctx->bar2_fd = open(IVSHMEM_RESOURCE2_PATH, O_RDWR);
  if (p_dev_ctx->bar2_fd < 0) {
    printf("Failed to open %s\n", IVSHMEM_RESOURCE2_PATH);
    return -1;
  }

  p_dev_ctx->shmem_size = _ivshmem_get_shmem_size();
  p_dev_ctx->p_shmem = mmap(NULL, p_dev_ctx->shmem_size, PROT_READ | PROT_WRITE,
                            MAP_SHARED, p_dev_ctx->bar2_fd, 0);
  if (p_dev_ctx->p_shmem == MAP_FAILED) {
    printf("Failed to map %s\n", IVSHMEM_RESOURCE2_PATH);
    close(p_dev_ctx->bar2_fd);
    return -1;
  }

  return 0;
}

void ivshmem_close_dev(struct IvshmemDeviceContext *p_dev_ctx) {
  if (p_dev_ctx->p_reg) {
    munmap(p_dev_ctx->p_reg, IVSHMEM_BAR0_SIZE);
    p_dev_ctx->p_reg = NULL;
  }

  if (p_dev_ctx->bar0_fd >= 0) {
    close(p_dev_ctx->bar0_fd);
    p_dev_ctx->bar0_fd = -1;
  }

  if (p_dev_ctx->p_shmem) {
    munmap(p_dev_ctx->p_shmem, p_dev_ctx->shmem_size);
    p_dev_ctx->p_shmem = NULL;
  }

  if (p_dev_ctx->bar2_fd >= 0) {
    close(p_dev_ctx->bar2_fd);
    p_dev_ctx->bar2_fd = -1;
  }
}


#include "ivshmem_lib.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <unistd.h>

int get_vm_id() { return 1; }

// Functions from ACRN project
void ivshmem_init_dev_ctx(struct IvshmemDeviceContext *p_dev_ctx) {
  int i;
  memset(p_dev_ctx, 0, sizeof(struct IvshmemDeviceContext));

  p_dev_ctx->vm_id = get_vm_id();
  p_dev_ctx->bar0_fd = -1;
  p_dev_ctx->p_reg = NULL;
  p_dev_ctx->bar2_fd = -1;
  p_dev_ctx->p_shmem = NULL;

  p_dev_ctx->io_fd = -1;

  for (i = 0; i < IVSHMEM_MAX_IRQ; i++) {
    p_dev_ctx->epfds_irq[i] = -1;
    p_dev_ctx->irq_data[i].fd = -1;
  }
  p_dev_ctx->is_opened = false;
}

int ivshmem_open_dev(struct IvshmemDeviceContext *p_dev_ctx) {
#if IVSHMEM_RUNNING_OS == 1  // SHM
  const char *shmName = "/mysharedmem";
  p_dev_ctx->bar0_fd = -1;
  p_dev_ctx->p_reg = NULL;
  p_dev_ctx->bar2_fd = shm_open("/mysharedmem", O_CREAT | O_RDWR, 0666);
  if (p_dev_ctx->bar2_fd == -1) {
    printf("Failed to create/open shared memory\n");
    return -1;
  }
  if (ftruncate(p_dev_ctx->bar2_fd, IVSHMEM_SIZE) == -1) {
    printf("Failed to set size for shared memory\n");
    return -1;
  }
  p_dev_ctx->p_shmem = mmap(NULL, IVSHMEM_SIZE, PROT_READ | PROT_WRITE,
                            MAP_SHARED, p_dev_ctx->bar2_fd, 0);

#else

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
  p_dev_ctx->p_shmem = mmap(NULL, IVSHMEM_SIZE, PROT_READ | PROT_WRITE,
                            MAP_SHARED, p_dev_ctx->bar2_fd, 0);

#endif

  if (p_dev_ctx->p_shmem == MAP_FAILED) {
    printf("Failed to map %s\n", IVSHMEM_RESOURCE2_PATH);
    close(p_dev_ctx->bar2_fd);
    return -1;
  }

  // eventfd_t evt_fd;
  // p_dev_ctx->io_fd = open(IVSHMEM_PCI_PATH, O_RDWR);
  // if (p_dev_ctx->io_fd < 0) {
  //   printf("Failed to open %s\n", IVSHMEM_PCI_PATH);
  //   return -1;
  // }
  // for (int i = 0; i < IVSHMEM_MAX_IRQ; i++) {
  //   struct epoll_event events;

  //   evt_fd = eventfd(0, EFD_NONBLOCK);
  //   p_dev_ctx->irq_data[i].vector = i;
  //   p_dev_ctx->irq_data[i].fd = evt_fd;

  //   /* create epoll */
  //   p_dev_ctx->epfds_irq[i] = epoll_create1(0);

  //   /* add eventfds of msix to epoll */
  //   events.events = EPOLLIN;
  //   events.data.ptr = &p_dev_ctx->irq_data[i];
  //   epoll_ctl(p_dev_ctx->epfds_irq[i], EPOLL_CTL_ADD, evt_fd, &events);

  //   p_dev_ctx->epfds_irq[i] = evt_fd;
  // }

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
    munmap(p_dev_ctx->p_shmem, IVSHMEM_SIZE);
    p_dev_ctx->p_shmem = NULL;
  }

  if (p_dev_ctx->bar2_fd >= 0) {
    close(p_dev_ctx->bar2_fd);
    p_dev_ctx->bar2_fd = -1;
  }

  // int i;
  // /* used for doorbell mode*/
  // for (i = 0; i < IVSHMEM_MAX_IRQ; i++) {
  //   close(p_dev_ctx->irq_data[i].fd);
  //   p_dev_ctx->irq_data[i].fd = -1;
  //   close(p_dev_ctx->epfds_irq[i]);
  //   p_dev_ctx->epfds_irq[i] = -1;
  // }
  // close(p_dev_ctx->io_fd);
  // p_dev_ctx->io_fd = -1;
}

int ivshmem_observe() {
  // wait for irq
}

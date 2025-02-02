#ifndef __IVSHMEM_LIB_H__
#define __IVSHMEM_LIB_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define IVSHMEM_RUNNING_OS 2  // 1: QEMU_HOST, 2: QEMU_VM

#if IVSHMEM_RUNNING_OS == 1  // QEMU_HOST
#define IVSHMEM_PCI_PATH "/dev/shm/ivshmem"
#define IVSHMEM_RESOURCE0_PATH "/dev/shm/ivshmem"
#define IVSHMEM_RESOURCE2_PATH "/dev/shm/ivshmem"
#elif IVSHMEM_RUNNING_OS == 2  // QEMU_VM
#define IVSHMEM_PCI_PATH "/sys/bus/pci/devices/0000:00:04.0"
#define IVSHMEM_RESOURCE0_PATH \
  "/sys/class/pci_bus/0000:00/device/0000:00:04.0/resource0"
#define IVSHMEM_RESOURCE2_PATH \
  "/sys/class/pci_bus/0000:00/device/0000:00:04.0/resource2"
#endif

#define IVSHMEM_MAX_IRQ 8
#define IVSHMEM_BAR0_SIZE 256
// #define IVSHMEM_PATH "~/ivshmem"
#define IVSHMEM_SIZE (4 * 1024 * 1024)
#define CONTROL_SECTION_SIZE (64 * 1024)
#define DATA_SECTION_SIZE (IVSHMEM_SIZE - CONTROL_SECTION_SIZE)
// Maximum number of IRQs.

struct IvshmemIrqData {
  int fd;
  int vector;
};

struct IvshmemDeviceContext {
  int vm_id;

  int bar0_fd;
  uint32_t *p_reg;

  int bar2_fd;
  uint32_t *p_shmem;

  size_t shmem_size;

  /* used for doorbell mode */
  int io_fd;
  int epfds_irq[IVSHMEM_MAX_IRQ];
  struct IvshmemIrqData irq_data[IVSHMEM_MAX_IRQ];
  bool is_opened;
};

int get_vm_id();

int ivshmem_open_fd();

void ivshmem_init_dev_ctx(struct IvshmemDeviceContext *p_dev_ctx);

uint32_t _ivshmem_get_shmem_size();

int ivshmem_open_dev(struct IvshmemDeviceContext *p_dev_ctx);

void ivshmem_close_dev(struct IvshmemDeviceContext *p_dev_ctx);

#endif

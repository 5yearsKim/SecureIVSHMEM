#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ivshmem_lib.h"
#include "ivshmem_lock_base.h"

int main() {
  int ret;
  struct IvshmemDeviceContext dev_ctx;
  struct IvshmemLockControlSection *p_ctr_sec;
  void *p_data;

  /* Initialize the device context and open the shared memory device */
  ivshmem_init_dev_ctx(&dev_ctx);
  ret = ivshmem_open_dev(&dev_ctx);
  if (ret != 0) {
    fprintf(stderr, "ivshmem_open_dev() failed\n");
    exit(EXIT_FAILURE);
  }

  /* Map the lock-based control section (layout: [control section][data
   * section]) */
  p_ctr_sec = (struct IvshmemLockControlSection *)dev_ctx.p_shmem;
  ret = ivshmem_lock_init_control_section(p_ctr_sec);
  if (ret != 0) {
    fprintf(stderr, "ivshmem_lock_init_control_section() failed\n");
    ivshmem_close_dev(&dev_ctx);
    exit(EXIT_FAILURE);
  }

  return 0;
}
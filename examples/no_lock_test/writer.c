#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "ivshmem_lib.h"
#include "ivshmem_secure.h"

/* Default VM IDs if not provided by the user */
#define DEFAULT_SENDER_VM 1
#define DEFAULT_RECEIVER_VM 2

int main(int argc, char **argv) {
  int ret;
  struct IvshmemDeviceContext dev_ctx;
  struct IvshmemControlSection *p_ctr;
  unsigned int sender_vm = DEFAULT_SENDER_VM;
  unsigned int receiver_vm = DEFAULT_RECEIVER_VM;

  /* Parse command-line options */
  int opt;
  int option_index = 0;

  /* Initialize device context and open the device */
  ivshmem_init_dev_ctx(&dev_ctx);
  ret = ivshmem_open_dev(&dev_ctx);
  if (ret != 0) {
    fprintf(stderr, "ivshmem_open_dev() failed\n");
    exit(EXIT_FAILURE);
  }

  /* Get the control section pointer */
  p_ctr = (struct IvshmemControlSection *)dev_ctx.p_shmem;

  /*
   * Create (or request) a channel.
   * Here we use a key with:
   *   sender_vm = <value from option>, sender_pid = current PID, receiver_vm =
   * <value from option>.
   */

  int message_size = IVSHMEM_SIZE - sizeof(struct IvshmemControlSection);

  /* Allocate a buffer to send the message */
  char *send_buf = malloc(message_size);
  if (send_buf == NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  unsigned long counter = 0;
  size_t total_bytes = 0;
  time_t start_time = time(NULL);

  while (1) {
    /* Prepare the message:
     * - The remainder is filled with a pattern (repeated byte equal to counter
     * % 256).
     */

    memset(send_buf, (char)(counter % 256), message_size);

    /* Copy the prepared message to the shared memory region */
    memcpy(dev_ctx.p_shmem, send_buf, message_size);

    total_bytes += message_size;
    counter++;

    usleep(1000 * 500);

    /* Every second, print the throughput in MB/s */
    time_t now = time(NULL);
    if (now - start_time >= 1) {
      double seconds = difftime(now, start_time);
      double mbps = (total_bytes / (1024.0 * 1024.0)) / seconds;
      printf(
          "\r\033[KSent %lu messages, throughput = %.2f MB/s, last counter = "
          "%lu",
          counter, mbps, counter);
      fflush(stdout);
      start_time = now;
      total_bytes = 0;
    }
  }

  free(send_buf);
  ivshmem_close_dev(&dev_ctx);
  return 0;
}

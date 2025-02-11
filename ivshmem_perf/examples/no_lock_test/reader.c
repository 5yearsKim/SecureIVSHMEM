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

/* Default sleep interval between polls (in seconds) */
#define DEFAULT_SLEEP_INTERVAL 1.0

int main(int argc, char **argv) {
  int ret;
  struct IvshmemDeviceContext dev_ctx;
  struct IvshmemControlSection *p_ctr;
  double sleep_interval = DEFAULT_SLEEP_INTERVAL;

  /* Allow the user to modify the sleep interval between polls via the -s option
   */
  int opt;
  while ((opt = getopt(argc, argv, "s:")) != -1) {
    switch (opt) {
      case 's':
        sleep_interval = atof(optarg);
        break;
      default:
        fprintf(stderr, "Usage: %s [-s sleep_interval_seconds]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
  }

  /* Initialize the shared-memory device context and open the device */
  ivshmem_init_dev_ctx(&dev_ctx);
  ret = ivshmem_open_dev(&dev_ctx);
  if (ret != 0) {
    fprintf(stderr, "ivshmem_open_dev() failed\n");
    exit(EXIT_FAILURE);
  }

  /* Get the control section pointer (if needed) */
  p_ctr = (struct IvshmemControlSection *)dev_ctx.p_shmem;

  /* Determine the size of the shared memory region (IVSHMEM_SIZE is defined in
   * ivshmem_lib) */
  int message_size = IVSHMEM_SIZE - sizeof(struct IvshmemControlSection);

  /* Allocate a local buffer to copy the shared memory contents */
  char *read_buf = malloc(message_size);
  if (read_buf == NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  /* Initialize the previous counter to a value that cannot be valid (e.g. -1)
   */
  int prev_counter = -1;
  size_t total_bytes = 0;
  time_t start_time = time(NULL);


  while (1) {
    /* Read the first byte from shared memory (our counter value) */
    int current = (int)(((unsigned char *)dev_ctx.p_shmem)[0]);

    /* If the counter has changed, copy the memory and verify its consistency */
    if (current == prev_counter) {
      continue;
    }
    usleep(10 * 1000);
    prev_counter = current;

    /* Copy the entire shared-memory region into our local buffer */
    memcpy(read_buf, dev_ctx.p_shmem, message_size);

    /* Verify that every byte in the local buffer equals the counter (current)
      */
    int consistent = 1;
    for (size_t i = 0; i < (size_t)message_size; i++) {
      if (((unsigned char *)read_buf)[i] != (unsigned char)current) {
        consistent = 0;
        fprintf(
            stderr,
            "\nData corruption detected at offset %zu: expected %u, got %u\n",
            i, current, ((unsigned char *)read_buf)[i]);
        break;
      }
    }
    if (consistent) {
      printf("consistent\n");
    }
    /* (Optional) Handle inconsistency as needed. Here we simply continue
      * polling. */

    total_bytes += message_size;

    /* Every second, calculate and print the throughput (MB/s) */
    time_t now = time(NULL);
    if (now - start_time >= 1) {
      double seconds = difftime(now, start_time);
      double mbps = (total_bytes / (1024.0 * 1024.0)) / seconds;
      printf("\r\033[KRead message with counter %d, throughput = %.2f MB/s",
             prev_counter, mbps);
      fflush(stdout);
      start_time = now;
      total_bytes = 0;
    }

    // /* Sleep for the specified interval before polling again */
    // usleep((useconds_t)(sleep_interval * 1000000));
  }

  free(read_buf);
  ivshmem_close_dev(&dev_ctx);
  return 0;
}

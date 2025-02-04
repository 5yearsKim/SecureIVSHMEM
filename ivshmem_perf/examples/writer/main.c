#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "ivshmem_data.h"
#include "ivshmem_lib.h"

#define MESSAGE_SIZE (256 * 1024 * 1024) /* Size of each message in bytes */

/* Default VM IDs if not provided by the user */
#define DEFAULT_SENDER_VM 1
#define DEFAULT_RECEIVER_VM 2

static void print_usage(const char *progname) {
  printf("Usage: %s [--sender_vm <id>] [--receiver_vm <id>]\n", progname);
  printf("  --sender_vm    Optional sender VM id (default %d)\n",
         DEFAULT_SENDER_VM);
  printf("  --receiver_vm  Optional receiver VM id (default %d)\n",
         DEFAULT_RECEIVER_VM);
}

int main(int argc, char **argv) {
  int ret;
  struct IvshmemDeviceContext dev_ctx;
  struct IvshmemControlSection *p_ctr;
  unsigned int sender_vm = DEFAULT_SENDER_VM;
  unsigned int receiver_vm = DEFAULT_RECEIVER_VM;

  /* Parse command-line options */
  int opt;
  int option_index = 0;
  static struct option long_options[] = {
      {"sender_vm", required_argument, 0, 's'},
      {"receiver_vm", required_argument, 0, 'r'},
      {"help", no_argument, 0, 'h'},
      {0, 0, 0, 0}};

  while ((opt = getopt_long(argc, argv, "s:r:h", long_options,
                            &option_index)) != -1) {
    switch (opt) {
      case 's':
        sender_vm = (unsigned int)atoi(optarg);
        break;
      case 'r':
        receiver_vm = (unsigned int)atoi(optarg);
        break;
      case 'h':
      default:
        print_usage(argv[0]);
        exit(EXIT_SUCCESS);
    }
  }

  printf("Using sender_vm=%u, receiver_vm=%u\n", sender_vm, receiver_vm);

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
  struct IvshmemChannelKey key = {.sender_vm = sender_vm,
                                  .sender_pid = getpid(),
                                  .receiver_vm = receiver_vm};

  /* Allocate a buffer to send the message */
  char *send_buf = malloc(MESSAGE_SIZE);
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
    memcpy(send_buf, &counter, sizeof(counter));
    memset(send_buf + sizeof(counter), (char)(counter % 256),
           MESSAGE_SIZE - sizeof(counter));

    ret = ivshmem_send_buffer(&key, p_ctr, send_buf, MESSAGE_SIZE);
    if (ret != 0) {
      fprintf(stderr, "ivshmem_send_buffer() failed\n");
    }
    total_bytes += MESSAGE_SIZE;
    counter++;

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

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

#define MESSAGE_SIZE                                      \
  (256 * 1024 * 1024) /* Must match writer's MESSAGE_SIZE \
                       */

/* Default values if arguments are not provided */
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
  void *p_data;
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

  printf("Using sender_vm = %u, receiver_vm = %u\n", sender_vm, receiver_vm);

  /* Initialize device context and open the device */
  ivshmem_init_dev_ctx(&dev_ctx);
  ret = ivshmem_open_dev(&dev_ctx);
  if (ret != 0) {
    fprintf(stderr, "ivshmem_open_dev() failed\n");
    exit(EXIT_FAILURE);
  }

  /* Get the control section and data section pointer */
  p_ctr = (struct IvshmemControlSection *)dev_ctx.p_shmem;
  p_data = ivshmem_get_data_section(p_ctr);

  /*
   * Look up the channel using the same key used by the writer.
   * Note: We set sender_pid = -1 to match any sender (since the reader
   * may not know the actual writer's PID).
   */
  struct IvshmemChannelKey key = {.sender_vm = sender_vm,
                                  .sender_pid = 0, /* accept any sender PID */
                                  .receiver_vm = receiver_vm};

  struct IvshmemChannel *channel = ivshmem_find_channel(p_ctr, &key);
  if (channel == NULL) {
    fprintf(stderr, "Channel not found\n");
    exit(EXIT_FAILURE);
  }
  printf("Channel found: buf_size = %zu, data_size = %zu\n", channel->buf_size,
         channel->data_size);

  /* Allocate a buffer for the received message */
  char *recv_buf = malloc(MESSAGE_SIZE);
  if (recv_buf == NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  unsigned long expected_counter = 0;
  size_t total_bytes = 0;
  time_t start_time = time(NULL);

  while (1) {
    /* Call the receive function using the channel pointer and the data section
     * pointer. This will copy MESSAGE_SIZE bytes from the shared memory into
     * recv_buf.
     */
    ret = ivshmem_recv_buffer(&key, p_ctr, recv_buf, MESSAGE_SIZE);
    if (ret != 0) {
      fprintf(stderr, "ivshmem_recv_buffer() failed\n");
      exit(EXIT_FAILURE);
    }

    total_bytes += MESSAGE_SIZE;

    /* Verify the message by reading the counter from the first 8 bytes */
    unsigned long received_counter = 0;
    memcpy(&received_counter, recv_buf, sizeof(received_counter));
    if (received_counter != expected_counter) {
      fprintf(stderr, "Data mismatch: expected %lu, got %lu\n",
              expected_counter, received_counter);
    }
    expected_counter++;

    /* Every second, print the throughput in MB/s */
    time_t now = time(NULL);
    if (now - start_time >= 1) {
      double seconds = difftime(now, start_time);
      double mbps = (total_bytes / (1024.0 * 1024.0)) / seconds;
      printf("Received messages, throughput = %.2f MB/s, last counter = %lu\n",
             mbps, received_counter);
      start_time = now;
      total_bytes = 0;
    }
  }

  free(recv_buf);
  ivshmem_close_dev(&dev_ctx);
  return 0;
}

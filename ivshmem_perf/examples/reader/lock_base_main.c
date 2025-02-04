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
#include "ivshmem_lock_base.h"

/* Message size set to 256 MB (must match the writer) */
#define MESSAGE_SIZE (256 * 1024 * 1024)

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
  struct IvshmemLockControlSection *p_ctr_sec;
  void *p_data;
  unsigned int sender_vm = DEFAULT_SENDER_VM;
  unsigned int receiver_vm = DEFAULT_RECEIVER_VM;

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

  printf("Lock Reader: Using sender_vm=%u, receiver_vm=%u\n", sender_vm,
         receiver_vm);

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
  p_data = ivshmem_lock_get_data_section(p_ctr_sec);

  /* Set up the key for the channel.
   * For the reader we use sender_pid = -1 to accept messages regardless of the
   * writer’s PID. We also ensure that the control section’s receiver_vm is set.
   */
  struct IvshmemLockKey key;
  key.sender_vm = sender_vm;
  key.sender_pid = 0; /* accept any sender PID */
  key.receiver_vm = receiver_vm;
  p_ctr_sec->key.receiver_vm = receiver_vm;

  /* Allocate a buffer for receiving the message */
  char *recv_buf = malloc(MESSAGE_SIZE);
  if (!recv_buf) {
    perror("malloc");
    ivshmem_close_dev(&dev_ctx);
    exit(EXIT_FAILURE);
  }

  unsigned long expected_counter = 0;
  size_t total_bytes = 0;
  time_t start_time = time(NULL);

  while (1) {
    ret = ivshmem_lock_recv_buffer(&key, p_ctr_sec, recv_buf, MESSAGE_SIZE);
    if (ret != 0) {
      /* No new data available; sleep a bit before retrying */
      usleep(10 * 1000);
      continue;
    }
    total_bytes += MESSAGE_SIZE;

    /* Verify the message by reading the counter from the first 8 bytes */
    unsigned long received_counter = 0;
    memcpy(&received_counter, recv_buf, sizeof(received_counter));
    if (received_counter != expected_counter) {
      fprintf(stderr, "Lock Reader: Data mismatch: expected %lu, got %lu\n",
              expected_counter, received_counter);
    }
    expected_counter++;

    time_t now = time(NULL);
    if (now - start_time >= 1) {
      double seconds = difftime(now, start_time);
      double mbps = (total_bytes / (1024.0 * 1024.0)) / seconds;
      printf(
          "Lock Reader: Received messages, throughput = %.2f MB/s, last "
          "counter = %lu\n",
          mbps, received_counter);
      start_time = now;
      total_bytes = 0;
    }
  }

  free(recv_buf);
  ivshmem_close_dev(&dev_ctx);
  return 0;
}

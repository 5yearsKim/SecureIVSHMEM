#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "ivshmem_lib.h"
#include "ivshmem_lock_base.h"

/* Message size set to 256 MB (must match the reader) */
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

  printf("Lock Writer: Using sender_vm=%u, receiver_vm=%u\n", sender_vm,
         receiver_vm);

  /* Initialize the device context and open the shared memory device */
  ivshmem_init_dev_ctx(&dev_ctx);
  ret = ivshmem_open_dev(&dev_ctx);
  if (ret != 0) {
    fprintf(stderr, "ivshmem_open_dev() failed\n");
    exit(EXIT_FAILURE);
  }

  /* Map the lock-based control section. The shared memory layout is assumed to
   * be: [ IvshmemLockControlSection ][ data section ]
   */
  p_ctr_sec = (struct IvshmemLockControlSection *)dev_ctx.p_shmem;

  // // init should be called on reader
  // ret = ivshmem_lock_init_control_section(p_ctr_sec);
  // if (ret != 0) {
  //   fprintf(stderr, "ivshmem_lock_init_control_section() failed\n");
  //   ivshmem_close_dev(&dev_ctx);
  //   exit(EXIT_FAILURE);
  // }

  p_data = ivshmem_lock_get_data_section(p_ctr_sec);

  /* Set up the key for the channel.
   * (For the lock-based API the control section carries a key.
   * Here we set it to the same sender_vm and receiver_vm values,
   * and use the current process’s PID for sender_pid.)
   */
  struct IvshmemLockKey key;
  key.sender_vm = sender_vm;
  key.sender_pid = getpid();
  key.receiver_vm = receiver_vm;
  p_ctr_sec->key = key;

  /* Allocate the buffer for the message */
  char *send_buf = malloc(MESSAGE_SIZE);
  if (!send_buf) {
    perror("malloc");
    ivshmem_close_dev(&dev_ctx);
    exit(EXIT_FAILURE);
  }

  unsigned long counter = 0;
  size_t total_bytes = 0;
  time_t start_time = time(NULL);

  while (1) {
    /* Prepare the message:
     * - The remainder is filled with a repeated byte pattern equal to counter %
     * 256.
     */
    memcpy(send_buf, &counter, sizeof(counter));
    memset(send_buf + sizeof(counter), (char)(counter % 256),
           MESSAGE_SIZE - sizeof(counter));

    ret = ivshmem_lock_send_buffer(&key, p_ctr_sec, send_buf, MESSAGE_SIZE);
    if (ret != 0) {
      fprintf(stderr, "ivshmem_lock_send_buffer() failed\n");
    }
    total_bytes += MESSAGE_SIZE;
    counter++;

    time_t now = time(NULL);
    if (now - start_time >= 1) {
      double seconds = difftime(now, start_time);
      double mbps = (total_bytes / (1024.0 * 1024.0)) / seconds;
      printf(
          "\r\033[KLock Writer: Sent messages, throughput = %.2f MB/s, last "
          "counter = %lu",
          mbps, counter);
      fflush(stdout);

      start_time = now;
      total_bytes = 0;
    }
  }

  free(send_buf);
  ivshmem_close_dev(&dev_ctx);
  return 0;
}

#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "ivshmem_data.h"
#include "ivshmem_lib.h"

#define MESSAGE_SIZE 256 * 1024 * 1024  /* Size of each message in bytes */
 
int main(void) {
    int ret;
    struct IvshmemDeviceContext dev_ctx;
    struct IvshmemControlSection *p_ctr;
    void *p_data;

    /* Initialize device context and open the device */
    ivshmem_init_dev_ctx(&dev_ctx);
    ret = ivshmem_open_dev(&dev_ctx);
    if (ret != 0) {
        fprintf(stderr, "ivshmem_open_dev() failed\n");
        exit(EXIT_FAILURE);
    }

    /* Get the control section and the data section pointer */
    p_ctr = (struct IvshmemControlSection *)dev_ctx.p_shmem;
    p_data = ivshmem_get_data_section(p_ctr);

    /*
     * Create (or request) a channel.
     * Here we use a key with:
     *   sender_vm = 1, sender_pid = current PID, receiver_vm = 2.
     */
    struct IvshmemChannelKey key = {
        .sender_vm = 1,
        .sender_pid = getpid(),
        .receiver_vm = 2
    };

    struct IvshmemChannel *channel = ivshmem_request_channel(p_ctr, &key);
    if (channel == NULL) {
        fprintf(stderr, "Failed to request channel\n");
        exit(EXIT_FAILURE);
    }
    printf("Channel acquired: buf_size = %zu, data_size = %zu\n",
           channel->buf_size, channel->data_size);

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
         * - The first 8 bytes hold the counter.
         * - The remainder is filled with a pattern (here, repeated byte equal to (counter % 256)).
         */


        memcpy(send_buf, &counter, sizeof(counter));
        memset(send_buf + sizeof(counter), (char)(counter % 256), MESSAGE_SIZE - sizeof(counter));

        // printf("Sending message %lu\n", counter);
        ret = ivshmem_send_buffer(channel, p_data, send_buf, MESSAGE_SIZE);
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
            printf("Sent %lu messages, throughput = %.2f MB/s, last counter = %lu\n",
                   counter, mbps, counter);
            start_time = now;
            total_bytes = 0;
            
        }
    }

    free(send_buf);
    ivshmem_close_dev(&dev_ctx);
    return 0;
}

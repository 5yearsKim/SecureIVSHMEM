// writer.c
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
#include "ivshmem_crypto.h"

#define MESSAGE_SIZE (256 * 1024 * 1024)  /* Size of plaintext in bytes */


static void print_usage(const char *progname) {
    printf("Usage: %s [--sender_vm <id>] [--receiver_vm <id>]\n", progname);
    printf("  --sender_vm    Optional sender VM id (default 1)\n");
    printf("  --receiver_vm  Optional receiver VM id (default 2)\n");
}

int main(int argc, char **argv) {
    int ret;
    struct IvshmemDeviceContext dev_ctx;
    struct IvshmemControlSection *p_ctr;
    unsigned int sender_vm = 1, receiver_vm = 2;

    // parse args
    int opt, option_index = 0;
    static struct option long_opts[] = {
        {"sender_vm",   required_argument, 0, 's'},
        {"receiver_vm", required_argument, 0, 'r'},
        {"help",        no_argument,       0, 'h'},
        {0,0,0,0}
    };
    while ((opt = getopt_long(argc, argv, "s:r:h", long_opts, &option_index)) != -1) {
        if (opt == 's') sender_vm = atoi(optarg);
        else if (opt == 'r') receiver_vm = atoi(optarg);
        else { print_usage(argv[0]); exit(EXIT_SUCCESS); }
    }
    printf("Using sender_vm=%u, receiver_vm=%u\n", sender_vm, receiver_vm);

    // init IVSHMEM
    ivshmem_init_dev_ctx(&dev_ctx);
    if ((ret = ivshmem_open_dev(&dev_ctx)) != 0) {
        fprintf(stderr, "ivshmem_open_dev() failed\n");
        exit(EXIT_FAILURE);
    }
    p_ctr = (struct IvshmemControlSection *)dev_ctx.p_shmem;

    // allocate buffers
    uint8_t *plaintext = malloc(MESSAGE_SIZE);
    uint8_t *cipher_buf = malloc(MESSAGE_SIZE);
    uint8_t *send_buf = malloc(IV_LEN + MESSAGE_SIZE + TAG_LEN);
    uint8_t  iv[IV_LEN], tag[TAG_LEN];

    if (!plaintext || !cipher_buf || !send_buf) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    static const uint8_t key[KEY_LEN] = {
      0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF,
      0x10,0x32,0x54,0x76,0x98,0xBA,0xDC,0xFE
    };


    unsigned long counter = 0;
    size_t total_bytes  = 0;
    size_t crypto_bytes = 0;
    time_t start_time   = time(NULL);
    double crypto_time_acc = 0.0;

    struct IvshmemChannelKey key_descr = {
        .sender_vm = sender_vm,
        .sender_pid = getpid(),
        .receiver_vm = receiver_vm
    };

    while (1) {
        // fill plaintext
        memcpy(plaintext, &counter, sizeof(counter));
        memset(plaintext + sizeof(counter), (char)(counter % 256),
               MESSAGE_SIZE - sizeof(counter));

        // encrypt
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        RAND_bytes(iv, IV_LEN);
        int ct_len = aes_gcm_encrypt(
            plaintext, MESSAGE_SIZE,
            NULL, 0,
            key, iv, IV_LEN,
            cipher_buf, tag
        );
        clock_gettime(CLOCK_MONOTONIC, &t1);
        crypto_bytes += MESSAGE_SIZE;
        crypto_time_acc += (t1.tv_sec - t0.tv_sec) +
                           (t1.tv_nsec - t0.tv_nsec)*1e-9;

        // pack IV || ciphertext || tag
        memcpy(send_buf, iv, IV_LEN);
        memcpy(send_buf + IV_LEN, cipher_buf, ct_len);
        memcpy(send_buf + IV_LEN + ct_len, tag, TAG_LEN);

        // send
        ret = ivshmem_send_buffer(&key_descr, p_ctr, send_buf,
                                  IV_LEN + ct_len + TAG_LEN);

        // printf("ct_len = %d\n", ct_len);
        if (ret) fprintf(stderr, "ivshmem_send_buffer() failed\n");

        total_bytes += ct_len;
        counter++;

        // report
        time_t now = time(NULL);
        if (now - start_time >= 1) {
            double elapsed = difftime(now, start_time);
            double net_mbps = (total_bytes / (1024.0*1024.0)) / elapsed;
            double crypto_mbps = (crypto_bytes / (1024.0*1024.0)) / crypto_time_acc;
            printf("\rSent %lu msgs | net = %.2f MB/s | crypto = %.2f MB/s",
                   counter, net_mbps, crypto_mbps);
            fflush(stdout);
            start_time = now;
            total_bytes = 0;
            crypto_bytes = 0;
            crypto_time_acc = 0.0;
        }
    }

    // cleanup (never reached)
    free(plaintext);
    free(cipher_buf);
    free(send_buf);
    ivshmem_close_dev(&dev_ctx);
    return 0;
}

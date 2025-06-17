// ivshmem_latency.c
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include <inttypes.h>  
#include <signal.h> 


#define SHM_DEV       "/dev/myshm"
#define ITERATIONS    100000

struct shm_region {
    volatile uint64_t seq;
    volatile uint64_t ack;
    volatile uint64_t data;
};

static inline uint64_t time_ns(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0) {
        perror("clock_gettime");
        exit(1);
    }
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

int main(void) {
    int fd = open(SHM_DEV, O_RDWR);
    if (fd < 0) {
        perror("open " SHM_DEV);
        return 1;
    }

    size_t pagesz = sysconf(_SC_PAGE_SIZE);
    struct shm_region *shm = mmap(NULL, pagesz,
                                  PROT_READ | PROT_WRITE,
                                  MAP_SHARED, fd, 0);
    if (shm == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }
    close(fd);

    // initialize handshake counters
    shm->seq = 0;
    shm->ack = 0;

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        munmap(shm, pagesz);
        return 1;
    }

    if (pid == 0) {
        // --- Child: the reader ---
        uint64_t last_seen = 0;
        for (;;) {
            // wait for new sequence
            while (shm->seq == last_seen) {
                ; // busy-wait
            }
            last_seen = shm->seq;

            // read the data
            uint64_t v = shm->data;
            // (here you could process v, copy it elsewhere, etc.)

            // notify parent by echoing seq
            shm->ack = last_seen;
        }
        // never reached
    } else {
        // --- Parent: the writer + timer ---
        uint64_t total_ns = 0;
        printf("Iter\tLatency (ns)\n");
        for (uint64_t i = 1; i <= ITERATIONS; i++) {
            // publish data
            shm->data = i;

            // bump sequence to signal reader
            __sync_synchronize();         // memory barrier
            shm->seq = i;

            // timestamp
            uint64_t t0 = time_ns();

            // wait for reader to ack
            while (shm->ack != i) {
                ; // busy-wait
            }
            uint64_t t1 = time_ns();

            uint64_t lat = t1 - t0;
            total_ns += lat;
            // printf("%4" PRIu64 "\t%" PRIu64 "\n", i, lat);
        }
        double avg = (double)total_ns / ITERATIONS;
        printf("Average round-trip latency: %.2f ns\n", avg);

        // done
        munmap(shm, pagesz);
        kill(pid, SIGTERM);  // terminate child
    }

    return 0;
}

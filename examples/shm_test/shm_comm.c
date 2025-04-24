// shm_bw_sem.c
#define _GNU_SOURCE
#include <fcntl.h>      // O_* constants
#include <semaphore.h>  // sem_t, sem_init, sem_wait, sem_post
#include <stdint.h>     // uint64_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>    // memcpy
#include <sys/mman.h>  // shm_open, mmap
#include <sys/stat.h>  // mode constants
#include <sys/time.h>  // gettimeofday
#include <unistd.h>    // fork, ftruncate

#define SHM_NAME "/my_shm_bw_sem"
#define CHUNK_SIZE (1 << 24)  // 16 MiB payload per transfer
#define SHM_SIZE (2 * sizeof(sem_t) + CHUNK_SIZE)

static inline double time_since(const struct timeval *t0) {
  struct timeval t1;
  gettimeofday(&t1, NULL);
  return (t1.tv_sec - t0->tv_sec) + (t1.tv_usec - t0->tv_usec) / 1e6;
}

int main(void) {
  // 1) create & size shared memory
  int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0600);
  if (fd < 0) {
    perror("shm_open");
    exit(1);
  }
  if (ftruncate(fd, SHM_SIZE) < 0) {
    perror("ftruncate");
    exit(1);
  }

  // 2) map it
  void *base = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (base == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }

  // 3) carve out semaphores + data
  sem_t *sem_w = (sem_t *)base;
  sem_t *sem_r = (sem_t *)((char *)base + sizeof(sem_t));
  uint8_t *data = (uint8_t *)base + 2 * sizeof(sem_t);

  // 4) init semaphores (pshared=1)
  sem_init(sem_w, 1, /*writer start unlocked=*/1);
  sem_init(sem_r, 1, /*reader start locked=*/0);

  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    exit(1);
  }

  if (pid == 0) {
    // ─── Writer ───
    uint64_t seq = 0;
    size_t bytes = 0;
    struct timeval last, now;
    gettimeofday(&last, NULL);

    while (1) {
      // wait until reader has consumed last chunk
      sem_wait(sem_w);

      // write sequence + pattern
      memcpy(data, &seq, sizeof(seq));
      memset(data + sizeof(seq), (uint8_t)(seq & 0xFF),
             CHUNK_SIZE - sizeof(seq));

      // signal reader
      sem_post(sem_r);

      bytes += CHUNK_SIZE;
      seq++;

      // report every second
      if (time_since(&last) >= 1.0) {
        gettimeofday(&now, NULL);
        double mbps = (bytes / 1024.0 / 1024.0) / time_since(&last);
        printf("[Writer] %.2f MB/s\n", mbps);
        fflush(stdout);
        bytes = 0;
        last = now;
      }
    }
  } else {
    // ─── Reader ───
    uint64_t expected = 0, seen;
    size_t bytes = 0;
    struct timeval last, now;
    gettimeofday(&last, NULL);

    while (1) {
      // wait until writer has produced a chunk
      sem_wait(sem_r);

      // read & verify
      memcpy(&seen, data, sizeof(seen));
      if (seen != expected) {
        fprintf(stderr, "[Reader] Seq mismatch: got %lu expected %lu\n",
                (unsigned long)seen, (unsigned long)expected);
        expected = seen + 1;
      } else {
        expected++;
      }

      // signal writer
      sem_post(sem_w);

      bytes += CHUNK_SIZE;

      // report every second
      if (time_since(&last) >= 1.0) {
        gettimeofday(&now, NULL);
        double mbps = (bytes / 1024.0 / 1024.0) / time_since(&last);
        printf("[Reader] %.2f MB/s\n", mbps);
        fflush(stdout);
        bytes = 0;
        last = now;
      }
    }
  }

  return 0;
}

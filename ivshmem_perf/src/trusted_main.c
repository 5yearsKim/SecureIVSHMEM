
#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

#include "ivshmem_data.h"
#include "ivshmem_lib.h"

int main() {
  uint32_t *p_data;
  struct IvshmemDeviceContext dev_ctx;
  struct IvshmemControlSection *p_ctr;
  int ep_fd, timer_fd, timer_fd2;
  struct itimerspec timer_spec;
  struct epoll_event ev, events[2];

  int i, ret;

  ep_fd = epoll_create1(0);
  if (ep_fd == -1) {
      perror("epoll_create1");
      exit(EXIT_FAILURE);
  }


  // Create a timerfd with CLOCK_MONOTONIC in non-blocking mode.
  timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
  if (timer_fd == -1) {
    perror("timerfd_create");
    exit(EXIT_FAILURE);
  }

  // Configure the timer to first expire after 10 seconds, then every 10
  // seconds.
  timer_spec.it_value.tv_sec = 1;  // Initial expiration:
  timer_spec.it_value.tv_nsec = 0;
  timer_spec.it_interval.tv_sec = 1;  // Interval: N seconds
  timer_spec.it_interval.tv_nsec = 0;

  if (timerfd_settime(timer_fd, 0, &timer_spec, NULL) == -1) {
    perror("timerfd_settime");
    close(timer_fd);
    exit(EXIT_FAILURE);
  }

  ev.events = EPOLLIN;
  ev.data.fd = timer_fd;

  ret = epoll_ctl(ep_fd, EPOLL_CTL_ADD, timer_fd, &ev);
  if (ret == -1) {
    perror("epoll_ctl: timer_fd");
    close(timer_fd);
    close(ep_fd);
    exit(EXIT_FAILURE);
  }


  // timer for monitoring lock value
  timer_fd2 = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
  if (timer_fd2 == -1) {
    perror("timerfd_create");
    exit(EXIT_FAILURE);
  }

  // Configure the timer to periodically expires every 10ms
  timer_spec.it_value.tv_sec = 0;  // Initial expiration:
  timer_spec.it_value.tv_nsec = 10000000;
  timer_spec.it_interval.tv_sec = 0;  // Interval: N seconds
  timer_spec.it_interval.tv_nsec = 10000000;

  if (timerfd_settime(timer_fd2, 0, &timer_spec, NULL) == -1) {
    perror("timerfd_settime");
    close(timer_fd2);
    exit(EXIT_FAILURE);
  }

  ev.events = EPOLLIN;
  ev.data.fd = timer_fd2;

  ret = epoll_ctl(ep_fd, EPOLL_CTL_ADD, timer_fd2, &ev);
  if (ret == -1) {
    perror("epoll_ctl: timer_fd2");
    close(timer_fd2);
    close(ep_fd);
    exit(EXIT_FAILURE);
  }


  // Also add standard input (fd 0) to epoll so we can handle user input.
  ev.events = EPOLLIN;
  ev.data.fd = STDIN_FILENO;
  ret = epoll_ctl(ep_fd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);
  if (ret == -1) {
    perror("epoll_ctl: STDIN");
    close(timer_fd);
    close(ep_fd);
    exit(EXIT_FAILURE);
  }

  /*
  ivshmem_init_dev_ctx(&dev_ctx);
  ret = ivshmem_open_dev(&dev_ctx);

  assert(ret == 0);

  p_ctr = (struct IvshmemControlSection *)dev_ctx.p_shmem;
  p_data = dev_ctx.p_shmem + CONTROL_SECTION_SIZE;
  */


  // Event loop: wait for events from both timerfd and stdin.
  while (1) {
    printf("Waiting for events...\n");
    int nfds =
        epoll_wait(ep_fd, events, 2, -1);  // Wait indefinitely for an event.
    if (nfds == -1) {
      if (errno == EINTR)
        continue;  // Interrupted by signal, restart epoll_wait.
      perror("epoll_wait");
      break;
    }

    for (int i = 0; i < nfds; i++) {
      if (events[i].data.fd == timer_fd) {
        // Timer event: read the number of expirations.
        uint64_t expirations;
        ssize_t s = read(timer_fd, &expirations, sizeof(expirations));
        if (s != sizeof(expirations)) {
          perror("read timerfd");
        }
        printf("Timer tick: %llu expiration(s) occurred\n",
               (unsigned long long)expirations);
        // Here you can perform your periodic task.
      } else if (events[i].data.fd == timer_fd2) {
        // Timer event: read the number of expirations.
        uint64_t expirations2;
        ssize_t s2 = read(timer_fd2, &expirations2, sizeof(expirations2));
        if (s2 != sizeof(expirations2)) {
          perror("read timerfd2");
        }
        printf("Timer tick: %llu expiration(s) occurred\n",
               (unsigned long long)expirations2);
        // Here you can perform your periodic task.
      }
      
      else if (events[i].data.fd == STDIN_FILENO) {
        // Standard input event: read and echo the input.
        char buf[128];
        ssize_t count = read(STDIN_FILENO, buf, sizeof(buf) - 1);
        if (count > 0) {
          buf[count] = '\0';
          printf("You entered: %s", buf);
        } else if (count == 0) {
          // End-of-file (e.g., Ctrl-D on Unix): exit the loop.
          printf("EOF on stdin, exiting...\n");
          goto cleanup;
        }
      }
    }
  }

cleanup:
  close(timer_fd);
  close(ep_fd);
  return 0;

  return 0;
}
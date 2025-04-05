

#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

#include "ivshmem_trusted.h"

#define RB_TIMER_INTERVAL_SEC 1    /* rebalancing every 1 second */
#define CTL_TIMER_INTERVAL_MSEC 100 /* control check every N msec */

int main() {
  struct IvshmemDeviceContext dev_ctx;
  int ep_fd, rb_timer_fd, ctl_timer_fd;
  struct itimerspec rb_timer_spec, ctl_timer_spec;
  struct epoll_event ev, events[3];
  struct IvshmemControlSection *p_ctr;
  void *p_data;
  int ret;

  printf("Initializing device context...\n");
  ivshmem_init_dev_ctx(&dev_ctx);
  printf("Opening device...\n");
  ret = ivshmem_open_dev(&dev_ctx);
  assert(ret == 0);



  p_ctr = (struct IvshmemControlSection *)dev_ctx.p_shmem;
  p_data = ivshmem_get_data_section(p_ctr);


  printf("Initializing control section...\n");
  ret = ivshmem_init_control_section(p_ctr, p_data);
  assert(ret == 0);

  /* Create epoll instance */
  ep_fd = epoll_create1(0);
  if (ep_fd == -1) {
    perror("epoll_create1");
    exit(EXIT_FAILURE);
  }

  /* Create timerfd for rebalancing */
  rb_timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
  if (rb_timer_fd == -1) {
    perror("timerfd_create (rb_timer_fd)");
    exit(EXIT_FAILURE);
  }
  rb_timer_spec.it_value.tv_sec = RB_TIMER_INTERVAL_SEC;
  rb_timer_spec.it_value.tv_nsec = 0;
  rb_timer_spec.it_interval.tv_sec = RB_TIMER_INTERVAL_SEC;
  rb_timer_spec.it_interval.tv_nsec = 0;
  if (timerfd_settime(rb_timer_fd, 0, &rb_timer_spec, NULL) == -1) {
    perror("timerfd_settime (rb_timer_fd)");
    exit(EXIT_FAILURE);
  }
  ev.events = EPOLLIN;
  ev.data.fd = rb_timer_fd;
  if (epoll_ctl(ep_fd, EPOLL_CTL_ADD, rb_timer_fd, &ev) == -1) {
    perror("epoll_ctl (rb_timer_fd)");
    exit(EXIT_FAILURE);
  }

  /* Create timerfd for control (channel creation) checks */
  ctl_timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
  if (ctl_timer_fd == -1) {
    perror("timerfd_create (ctl_timer_fd)");
    exit(EXIT_FAILURE);
  }
  /* Set to fire every 10 msec */
  ctl_timer_spec.it_value.tv_sec = 0;
  ctl_timer_spec.it_value.tv_nsec = CTL_TIMER_INTERVAL_MSEC * 1000000;
  ctl_timer_spec.it_interval.tv_sec = 0;
  ctl_timer_spec.it_interval.tv_nsec = CTL_TIMER_INTERVAL_MSEC * 1000000;
  if (timerfd_settime(ctl_timer_fd, 0, &ctl_timer_spec, NULL) == -1) {
    perror("timerfd_settime (ctl_timer_fd)");
    exit(EXIT_FAILURE);
  }
  ev.events = EPOLLIN;
  ev.data.fd = ctl_timer_fd;
  if (epoll_ctl(ep_fd, EPOLL_CTL_ADD, ctl_timer_fd, &ev) == -1) {
    perror("epoll_ctl (ctl_timer_fd)");
    exit(EXIT_FAILURE);
  }

  /* Optionally, add STDIN for debug commands */
  ev.events = EPOLLIN;
  ev.data.fd = STDIN_FILENO;
  if (epoll_ctl(ep_fd, EPOLL_CTL_ADD, STDIN_FILENO, &ev) == -1) {
    perror("epoll_ctl (STDIN_FILENO)");
    exit(EXIT_FAILURE);
  }

  printf("Trusted host main loop starting...\n");

  while (1) {
    int nfds = epoll_wait(ep_fd, events, 3, -1);
    if (nfds == -1) {
      if (errno == EINTR) continue;
      perror("epoll_wait");
      break;
    }

    for (int i = 0; i < nfds; i++) {
      if (events[i].data.fd == rb_timer_fd) {
        uint64_t expirations;
        ssize_t s = read(rb_timer_fd, &expirations, sizeof(expirations));
        if (s != sizeof(expirations)) {
          perror("read rb_timer_fd");
        }
        ivshmem_trusted_rebalancing(p_ctr, p_data);
      } else if (events[i].data.fd == ctl_timer_fd) {
        uint64_t expirations;
        ssize_t s = read(ctl_timer_fd, &expirations, sizeof(expirations));
        if (s != sizeof(expirations)) {
          perror("read ctl_timer_fd");
        }
        /* Check for pending control requests */
        if (ivshmem_check_is_control_requested(p_ctr)) {
          printf("Control request detected. Processing...\n");
          ivshmem_trusted_control(p_ctr);
        }
      } else if (events[i].data.fd == STDIN_FILENO) {
        char buf[128];
        ssize_t count = read(STDIN_FILENO, buf, sizeof(buf) - 1);
        if (count > 0) {
          buf[count] = '\0';
          printf("STDIN: %s", buf);
        }
      }
    }
  }

cleanup:

  close(rb_timer_fd);
  close(ctl_timer_fd);
  close(ep_fd);
  ivshmem_close_dev(&dev_ctx);
  return 0;
}

/*
 * main.c
 *
 * Copyright 6WIND S.A., 2014
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or
 * (at your option) any later version.
 */

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// #define _XOPEN_SOURCE 200    // 값에 따라 sigaction 정의가 달라집니다.

#include <signal.h>

#include "ivshmem-client.h"

#define IVSHMEM_CLIENT_DEFAULT_VERBOSE 0
#define IVSHMEM_CLIENT_DEFAULT_UNIX_SOCK_PATH "/tmp/ivshmem_socket"

typedef struct IvshmemClientArgs {
  int verbose;
  const char *unix_sock_path;
} IvshmemClientArgs;

/* Print usage information and exit */
static void ivshmem_client_usage(const char *name, int code) {
  fprintf(stderr, "Usage: %s [opts]\n", name);
  fprintf(stderr, "  -h: show this help\n");
  fprintf(stderr, "  -v: verbose mode\n");
  fprintf(stderr,
          "  -S <unix_sock_path>: path to the unix socket to connect to.\n");
  fprintf(stderr, "     default=%s\n", IVSHMEM_CLIENT_DEFAULT_UNIX_SOCK_PATH);
  exit(code);
}

/* Parse command-line arguments */
static void ivshmem_client_parse_args(IvshmemClientArgs *args, int argc,
                                      char *argv[]) {
  int c;
  while ((c = getopt(argc, argv, "hvS:")) != -1) {
    switch (c) {
      case 'h':
        ivshmem_client_usage(argv[0], 0);
        break;
      case 'v':
        args->verbose = 1;
        break;
      case 'S':
        args->unix_sock_path = optarg;
        break;
      default:
        ivshmem_client_usage(argv[0], 1);
        break;
    }
  }
}

/* Print available commands */
static void ivshmem_client_cmdline_help(void) {
  printf(
      "Commands:\n"
      "  dump                  - dump peers (including local info)\n"
      "  int <peer> <vector>    - notify one vector on a peer\n"
      "  int <peer> all        - notify all vectors of a peer\n"
      "  int all               - notify all vectors of all peers\n"
      "  help, ?             - show this help\n");
}

/* Read and process a command from stdin */
static int ivshmem_client_handle_stdin_command(IvshmemClient *client) {
  IvshmemClientPeer *peer;
  char buf[128];
  char *s, *token;
  int ret;
  int peer_id, vector;

  memset(buf, 0, sizeof(buf));
  ret = read(STDIN_FILENO, buf, sizeof(buf) - 1);
  if (ret < 0) {
    return -1;
  }
  s = buf;
  while ((token = strsep(&s, "\n\r;")) != NULL) {
    if (strcmp(token, "") == 0) continue;
    if (strcmp(token, "?") == 0 || strcmp(token, "help") == 0) {
      ivshmem_client_cmdline_help();
    } else if (strcmp(token, "dump") == 0) {
      ivshmem_client_dump(client);
    } else if (strcmp(token, "int all") == 0) {
      ivshmem_client_notify_broadcast(client);
    } else if (sscanf(token, "int %d %d", &peer_id, &vector) == 2) {
      peer = ivshmem_client_search_peer(client, peer_id);
      if (peer == NULL) {
        printf("cannot find peer_id = %d\n", peer_id);
        continue;
      }
      ivshmem_client_notify(client, peer, vector);
    } else if (sscanf(token, "int %d all", &peer_id) == 1) {
      peer = ivshmem_client_search_peer(client, peer_id);
      if (peer == NULL) {
        printf("cannot find peer_id = %d\n", peer_id);
        continue;
      }
      ivshmem_client_notify_all_vects(client, peer);
    } else {
      printf("invalid command, type help\n");
    }
  }
  printf("cmd> ");
  fflush(stdout);
  return 0;
}

/* Poll events from stdin and the ivshmem client file descriptors */
static int ivshmem_client_poll_events(IvshmemClient *client) {
  fd_set fds;
  int ret, maxfd;

  while (1) {
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    maxfd = STDIN_FILENO + 1;

    ivshmem_client_get_fds(client, &fds, &maxfd);

    ret = select(maxfd, &fds, NULL, NULL, NULL);
    if (ret < 0) {
      if (errno == EINTR) continue;
      perror("select error");
      break;
    }
    if (FD_ISSET(STDIN_FILENO, &fds) &&
        ivshmem_client_handle_stdin_command(client) < 0 && errno != EINTR) {
      fprintf(stderr, "ivshmem_client_handle_stdin_command() failed\n");
      break;
    }
    if (ivshmem_client_handle_fds(client, &fds, maxfd) < 0) {
      fprintf(stderr, "ivshmem_client_handle_fds() failed\n");
      break;
    }
  }
  return ret;
}

/* Notification callback invoked when a notification is received */
static void ivshmem_client_notification_cb(const IvshmemClient *client,
                                           const IvshmemClientPeer *peer,
                                           unsigned vect, void *arg) {
  (void)client;
  (void)arg;
  printf("receive notification from peer_id=%" PRId64 " vector=%u\n", peer->id,
         vect);
}

int main(int argc, char *argv[]) {
  struct sigaction sa;
  IvshmemClient client;
  IvshmemClientArgs args = {
      .verbose = IVSHMEM_CLIENT_DEFAULT_VERBOSE,
      .unix_sock_path = IVSHMEM_CLIENT_DEFAULT_UNIX_SOCK_PATH,
  };

  /* Parse arguments */
  ivshmem_client_parse_args(&args, argc, argv);

  /* Ignore SIGPIPE */
  sa.sa_handler = SIG_IGN;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGPIPE, &sa, 0) == -1) {
    perror("failed to ignore SIGPIPE; sigaction");
    return 1;
  }

  ivshmem_client_cmdline_help();
  printf("cmd> ");
  fflush(stdout);

  if (ivshmem_client_init(&client, args.unix_sock_path,
                          ivshmem_client_notification_cb, NULL,
                          args.verbose) < 0) {
    fprintf(stderr, "cannot init client\n");
    return 1;
  }

  while (1) {
    if (ivshmem_client_connect(&client) < 0) {
      fprintf(stderr, "cannot connect to server, retry in 1 second\n");
      sleep(1);
      continue;
    }

    fprintf(stdout, "listening on server socket %d\n", client.sock_fd);

    if (ivshmem_client_poll_events(&client) == 0) {
      continue;
    }

    fprintf(stdout, "disconnected from server\n");
    ivshmem_client_close(&client);
  }

  return 0;
}

/*
 * ivshmem-client.c
 *
 * Copyright 6WIND S.A., 2014
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or
 * (at your option) any later version.
 */

#include "ivshmem-client.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

/* Debug logging macro */
#define IVSHMEM_CLIENT_DEBUG(client, fmt, ...) \
  do {                                         \
    if ((client)->verbose) {                   \
      printf(fmt, ##__VA_ARGS__);              \
    }                                          \
  } while (0)

/* Read one message from the UNIX socket.
 * It reads an int64_t and optionally an ancillary fd.
 */
static int ivshmem_client_read_one_msg(IvshmemClient *client, int64_t *index,
                                       int *fd) {
  int ret;
  struct msghdr msg;
  struct iovec iov[1];
  union {
    struct cmsghdr cmsg;
    char control[CMSG_SPACE(sizeof(int))];
  } msg_control;
  struct cmsghdr *cmsg;

  iov[0].iov_base = index;
  iov[0].iov_len = sizeof(*index);

  memset(&msg, 0, sizeof(msg));
  msg.msg_iov = iov;
  msg.msg_iovlen = 1;
  msg.msg_control = &msg_control;
  msg.msg_controllen = sizeof(msg_control);

  ret = recvmsg(client->sock_fd, &msg, 0);
  if (ret < (int)sizeof(*index)) {
    IVSHMEM_CLIENT_DEBUG(client, "cannot read message: %s\n", strerror(errno));
    return -1;
  }
  if (ret == 0) {
    IVSHMEM_CLIENT_DEBUG(client, "lost connection to server\n");
    return -1;
  }

  *index = GINT64_FROM_LE(*index);
  *fd = -1;

  for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
    if (cmsg->cmsg_len != CMSG_LEN(sizeof(int)) ||
        cmsg->cmsg_level != SOL_SOCKET || cmsg->cmsg_type != SCM_RIGHTS) {
      continue;
    }
    memcpy(fd, CMSG_DATA(cmsg), sizeof(*fd));
  }

  return 0;
}

/* Free a peer structure and close its fds. */
static void ivshmem_client_free_peer(IvshmemClient *client,
                                     IvshmemClientPeer *peer) {
  unsigned vector;

  QTAILQ_REMOVE(&client->peer_list, peer, next);
  for (vector = 0; vector < peer->vectors_count; vector++) {
    close(peer->vectors[vector]);
  }
  free(peer);
}

/* Handle a message from the server (for new peer, new vector, or removal). */
static int ivshmem_client_handle_server_msg(IvshmemClient *client) {
  IvshmemClientPeer *peer;
  int64_t peer_id;
  int ret, fd;

  ret = ivshmem_client_read_one_msg(client, &peer_id, &fd);
  if (ret < 0) {
    return -1;
  }

  /* Find the peer; note that our own client is stored in client->local */
  peer = ivshmem_client_search_peer(client, peer_id);

  /* If fd == -1, this is a deletion message */
  if (fd == -1) {
    if (peer == NULL || peer == &client->local) {
      IVSHMEM_CLIENT_DEBUG(client,
                           "received delete for invalid "
                           "peer %" PRId64 "\n",
                           peer_id);
      return -1;
    }
    IVSHMEM_CLIENT_DEBUG(client, "delete peer id = %" PRId64 "\n", peer_id);
    ivshmem_client_free_peer(client, peer);
    return 0;
  }

  /* If no peer exists, create one */
  if (peer == NULL) {
    peer = calloc(1, sizeof(*peer));
    if (!peer) {
      return -1;
    }
    peer->id = peer_id;
    peer->vectors_count = 0;
    QTAILQ_INSERT_TAIL(&client->peer_list, peer, next);
    IVSHMEM_CLIENT_DEBUG(client, "new peer id = %" PRId64 "\n", peer_id);
  }

  /* Add new vector for the peer */
  IVSHMEM_CLIENT_DEBUG(client,
                       "  new vector %d (fd=%d) for peer id = %" PRId64 "\n",
                       peer->vectors_count, fd, peer->id);
  if (peer->vectors_count >= G_N_ELEMENTS(peer->vectors)) {
    IVSHMEM_CLIENT_DEBUG(client, "Too many vectors received, failing\n");
    return -1;
  }
  peer->vectors[peer->vectors_count] = fd;
  peer->vectors_count++;

  return 0;
}

/* Initialize a new ivshmem client structure */
int ivshmem_client_init(IvshmemClient *client, const char *unix_sock_path,
                        IvshmemClientNotifCb notif_cb, void *notif_arg,
                        bool verbose) {
  int ret;
  unsigned i;

  memset(client, 0, sizeof(*client));

  ret = snprintf(client->unix_sock_path, sizeof(client->unix_sock_path), "%s",
                 unix_sock_path);
  if (ret < 0 || ret >= (int)sizeof(client->unix_sock_path)) {
    IVSHMEM_CLIENT_DEBUG(client, "could not copy unix socket path\n");
    return -1;
  }

  for (i = 0; i < IVSHMEM_CLIENT_MAX_VECTORS; i++) {
    client->local.vectors[i] = -1;
  }

  QTAILQ_INIT(&client->peer_list);
  client->local.id = -1;

  client->notif_cb = notif_cb;
  client->notif_arg = notif_arg;
  client->verbose = verbose;
  client->shm_fd = -1;
  client->sock_fd = -1;

  return 0;
}

/* Connect to the ivshmem server via UNIX socket */
int ivshmem_client_connect(IvshmemClient *client) {
  struct sockaddr_un s_un;
  int ret, fd;
  int64_t tmp;

  IVSHMEM_CLIENT_DEBUG(client, "connect to server %s\n",
                       client->unix_sock_path);

  client->sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (client->sock_fd < 0) {
    IVSHMEM_CLIENT_DEBUG(client, "cannot create socket: %s\n", strerror(errno));
    return -1;
  }

  s_un.sun_family = AF_UNIX;
  ret = snprintf(s_un.sun_path, sizeof(s_un.sun_path), "%s",
                 client->unix_sock_path);
  if (ret < 0 || ret >= (int)sizeof(s_un.sun_path)) {
    IVSHMEM_CLIENT_DEBUG(client, "could not copy unix socket path\n");
    goto err_close;
  }

  if (connect(client->sock_fd, (struct sockaddr *)&s_un, sizeof(s_un)) < 0) {
    IVSHMEM_CLIENT_DEBUG(client, "cannot connect to %s: %s\n", s_un.sun_path,
                         strerror(errno));
    goto err_close;
  }

  /* First message: protocol version */
  if (ivshmem_client_read_one_msg(client, &tmp, &fd) < 0 || (tmp != 0) ||
      fd != -1) { /* assume protocol version is 0 */
    IVSHMEM_CLIENT_DEBUG(client,
                         "cannot read from server (protocol version)\n");
    goto err_close;
  }

  /* Second message: our client id */
  if (ivshmem_client_read_one_msg(client, &client->local.id, &fd) < 0 ||
      client->local.id < 0 || fd != -1) {
    IVSHMEM_CLIENT_DEBUG(client, "cannot read from server (client id)\n");
    goto err_close;
  }
  IVSHMEM_CLIENT_DEBUG(client, "our_id=%" PRId64 "\n", client->local.id);

  /* Third message: shared memory fd (sent with a -1 index) */
  if (ivshmem_client_read_one_msg(client, &tmp, &fd) < 0 || tmp != -1 ||
      fd < 0) {
    if (fd >= 0) {
      close(fd);
    }
    IVSHMEM_CLIENT_DEBUG(client, "cannot read from server (shm fd)\n");
    goto err_close;
  }
  client->shm_fd = fd;
  IVSHMEM_CLIENT_DEBUG(client, "shm_fd=%d\n", fd);

  return 0;

err_close:
  close(client->sock_fd);
  client->sock_fd = -1;
  return -1;
}

/* Close connection to the server and free all peer structures */
void ivshmem_client_close(IvshmemClient *client) {
  IvshmemClientPeer *peer;
  unsigned i;

  IVSHMEM_CLIENT_DEBUG(client, "close client\n");

  while ((peer = QTAILQ_FIRST(&client->peer_list)) != NULL) {
    ivshmem_client_free_peer(client, peer);
  }

  if (client->shm_fd >= 0) {
    close(client->shm_fd);
    client->shm_fd = -1;
  }
  if (client->sock_fd >= 0) {
    close(client->sock_fd);
    client->sock_fd = -1;
  }
  client->local.id = -1;
  for (i = 0; i < IVSHMEM_CLIENT_MAX_VECTORS; i++) {
    if (client->local.vectors[i] >= 0) {
      close(client->local.vectors[i]);
      client->local.vectors[i] = -1;
    }
  }
  client->local.vectors_count = 0;
}

/* Add our own fds (server socket and our event vectors) to fd_set */
void ivshmem_client_get_fds(const IvshmemClient *client, fd_set *fds,
                            int *maxfd) {
  int fd;
  unsigned vector;

  FD_SET(client->sock_fd, fds);
  if (client->sock_fd >= *maxfd) {
    *maxfd = client->sock_fd + 1;
  }

  for (vector = 0; vector < client->local.vectors_count; vector++) {
    fd = client->local.vectors[vector];
    FD_SET(fd, fds);
    if (fd >= *maxfd) {
      *maxfd = fd + 1;
    }
  }
}

/* Handle notifications on our event file descriptors */
static int ivshmem_client_handle_event(IvshmemClient *client, const fd_set *cur,
                                       int maxfd) {
  IvshmemClientPeer *peer = &client->local;
  uint64_t kick;
  unsigned i;
  int ret;

  for (i = 0; i < peer->vectors_count; i++) {
    if (peer->vectors[i] >= maxfd || !FD_ISSET(peer->vectors[i], cur)) {
      continue;
    }

    ret = read(peer->vectors[i], &kick, sizeof(kick));
    if (ret < 0) {
      return ret;
    }
    if (ret != (int)sizeof(kick)) {
      IVSHMEM_CLIENT_DEBUG(client, "invalid read size = %d\n", ret);
      errno = EINVAL;
      return -1;
    }
    IVSHMEM_CLIENT_DEBUG(client,
                         "received event on fd %d vector %u: %" PRIu64 "\n",
                         peer->vectors[i], i, kick);
    if (client->notif_cb != NULL) {
      client->notif_cb(client, peer, i, client->notif_arg);
    }
  }
  return 0;
}

/* Read and handle messages from the server and event file descriptors */
int ivshmem_client_handle_fds(IvshmemClient *client, fd_set *fds, int maxfd) {
  if (client->sock_fd < maxfd && FD_ISSET(client->sock_fd, fds) &&
      ivshmem_client_handle_server_msg(client) < 0 && errno != EINTR) {
    IVSHMEM_CLIENT_DEBUG(client, "ivshmem_client_handle_server_msg() failed\n");
    return -1;
  } else if (ivshmem_client_handle_event(client, fds, maxfd) < 0 &&
             errno != EINTR) {
    IVSHMEM_CLIENT_DEBUG(client, "ivshmem_client_handle_event() failed\n");
    return -1;
  }
  return 0;
}

/* Send a notification on a specific vector of a peer */
int ivshmem_client_notify(const IvshmemClient *client,
                          const IvshmemClientPeer *peer, unsigned vector) {
  uint64_t kick;
  int fd;

  if (vector >= peer->vectors_count) {
    IVSHMEM_CLIENT_DEBUG(client, "invalid vector %u on peer %" PRId64 "\n",
                         vector, peer->id);
    return -1;
  }
  fd = peer->vectors[vector];
  IVSHMEM_CLIENT_DEBUG(client, "notify peer %" PRId64 " on vector %u, fd %d\n",
                       peer->id, vector, fd);

  kick = 1;
  if (write(fd, &kick, sizeof(kick)) != (ssize_t)sizeof(kick)) {
    fprintf(stderr, "could not write to %d: %s\n", fd, strerror(errno));
    return -1;
  }
  return 0;
}

/* Send a notification to all vectors of a peer */
int ivshmem_client_notify_all_vects(const IvshmemClient *client,
                                    const IvshmemClientPeer *peer) {
  unsigned vector;
  int ret = 0;

  for (vector = 0; vector < peer->vectors_count; vector++) {
    if (ivshmem_client_notify(client, peer, vector) < 0) {
      ret = -1;
    }
  }
  return ret;
}

/* Broadcast a notification to all peers */
int ivshmem_client_notify_broadcast(const IvshmemClient *client) {
  IvshmemClientPeer *peer;
  int ret = 0;

  QTAILQ_FOREACH(peer, &client->peer_list, next) {
    if (ivshmem_client_notify_all_vects(client, peer) < 0) {
      ret = -1;
    }
  }
  return ret;
}

/* Look up a peer by its id */
IvshmemClientPeer *ivshmem_client_search_peer(IvshmemClient *client,
                                              int64_t peer_id) {
  IvshmemClientPeer *peer;

  if (peer_id == client->local.id) {
    return &client->local;
  }
  QTAILQ_FOREACH(peer, &client->peer_list, next) {
    if (peer->id == peer_id) {
      return peer;
    }
  }
  return NULL;
}

/* Dump information about our client and peers to stdout */
void ivshmem_client_dump(const IvshmemClient *client) {
  const IvshmemClientPeer *peer;
  unsigned vector;

  /* Dump local information */
  peer = &client->local;
  printf("our_id = %" PRId64 "\n", peer->id);
  for (vector = 0; vector < peer->vectors_count; vector++) {
    printf("  vector %u is enabled (fd=%d)\n", vector, peer->vectors[vector]);
  }

  /* Dump peers */
  QTAILQ_FOREACH(peer, &client->peer_list, next) {
    printf("peer_id = %" PRId64 "\n", peer->id);
    for (vector = 0; vector < peer->vectors_count; vector++) {
      printf("  vector %u is enabled (fd=%d)\n", vector, peer->vectors[vector]);
    }
  }
}

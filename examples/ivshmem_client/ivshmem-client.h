#ifndef IVSHMEM_CLIENT_H
#define IVSHMEM_CLIENT_H

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/select.h>

/* Include our compatibility headers */
#include "compat.h"
#include "queue_compat.h"

/**
 * Maximum number of notification vectors supported by the client
 */
#define IVSHMEM_CLIENT_MAX_VECTORS 64

/**
 * Structure storing a peer
 *
 * Each time a client connects to an ivshmem server, it is advertised to
 * all connected clients through the unix socket. When our ivshmem
 * client receives a notification, it creates a IvshmemClientPeer
 * structure to store the infos of this peer.
 *
 * This structure is also used to store the information of our own
 * client in (IvshmemClient)->local.
 */
typedef struct IvshmemClientPeer {
  QTAILQ_ENTRY(IvshmemClientPeer) next;    /**< next in list */
  int64_t id;                              /**< the id of the peer */
  int vectors[IVSHMEM_CLIENT_MAX_VECTORS]; /**< one fd per vector */
  unsigned vectors_count;                  /**< number of vectors */
} IvshmemClientPeer;

struct IvshmemClient;

/**
 * Typedef of callback function used when our IvshmemClient receives a
 * notification from a peer.
 */
typedef void (*IvshmemClientNotifCb)(const struct IvshmemClient *client,
                                     const IvshmemClientPeer *peer,
                                     unsigned vect, void *arg);

/**
 * Structure describing an ivshmem client
 *
 * This structure stores all information related to our client: the name
 * of the server unix socket, the list of peers advertised by the
 * server, our own client information, and a pointer to the notification
 * callback function used when we receive a notification from a peer.
 */
typedef struct IvshmemClient {
  char unix_sock_path[PATH_MAX]; /**< path to unix socket */
  int sock_fd;                   /**< unix sock file descriptor */
  int shm_fd;                    /**< shared memory file descriptor */

  QTAILQ_HEAD(, IvshmemClientPeer) peer_list; /**< list of peers */
  IvshmemClientPeer local;                    /**< our own infos */

  IvshmemClientNotifCb notif_cb; /**< notification callback */
  void *notif_arg;               /**< notification argument */

  bool verbose; /**< true to enable debug output */
} IvshmemClient;

/* Function prototypes */

/* Initialize an ivshmem client. */
int ivshmem_client_init(IvshmemClient *client, const char *unix_sock_path,
                        IvshmemClientNotifCb notif_cb, void *notif_arg,
                        bool verbose);

/* Connect to the server. */
int ivshmem_client_connect(IvshmemClient *client);

/* Close connection to the server and free all peer structures. */
void ivshmem_client_close(IvshmemClient *client);

/* Fill a fd_set with file descriptors to be monitored. */
void ivshmem_client_get_fds(const IvshmemClient *client, fd_set *fds,
                            int *maxfd);

/* Read and handle new messages from the server or peers. */
int ivshmem_client_handle_fds(IvshmemClient *client, fd_set *fds, int maxfd);

/* Send a notification to a vector of a peer. */
int ivshmem_client_notify(const IvshmemClient *client,
                          const IvshmemClientPeer *peer, unsigned vector);

/* Send a notification to all vectors of a peer. */
int ivshmem_client_notify_all_vects(const IvshmemClient *client,
                                    const IvshmemClientPeer *peer);

/* Broadcast a notification to all vectors of all peers. */
int ivshmem_client_notify_broadcast(const IvshmemClient *client);

/* Search for a peer given its id. */
IvshmemClientPeer *ivshmem_client_search_peer(IvshmemClient *client,
                                              int64_t peer_id);

/* Dump information of this ivshmem client on stdout. */
void ivshmem_client_dump(const IvshmemClient *client);

#endif /* IVSHMEM_CLIENT_H */

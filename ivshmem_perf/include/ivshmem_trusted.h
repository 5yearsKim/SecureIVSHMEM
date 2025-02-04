#ifndef __IVSHMEM_TRUSTED__
#define __IVSHMEM_TRUSTED__

#include <stdbool.h>

#include "ivshmem_secure.h"
#include "ivshmem_lib.h"

/* Kill a channel if it is inactive for this many seconds */
#define IVSHMEM_CHANNEL_KILL_THRESHOLD 5
/* Rebalance interval in milliseconds */
#define IVSHMEM_CHANNEL_REBALANCE_INTERVAL (10 * 1000)



/* Initial channel buffer size */
#define IVSHMEM_CHANNEL_INIT_SIZE (4 * IVSHMEM_PAGE_SIZE)

#define IVSHMEM_CHANNEL_ADD_VALUE (1 * IVSHMEM_PAGE_SIZE)

#define IVSHMEM_CHANNEL_DECREASE_MULTIPLIER 0.8

/* Initialize the control section with the provided data section pointer */
int ivshmem_init_control_section(struct IvshmemControlSection *p_ctr_sec,
                                 void *p_data);

/* Check if there is a pending channel request from an untrusted VM */
int ivshmem_check_is_control_requested(struct IvshmemControlSection *p_ctr_sec);

/* Perform periodic rebalancing on active channels:
 * - Increase buffer size for active channels.
 * - Kill channels that have been inactive for too long.
 */
int ivshmem_trusted_rebalancing(struct IvshmemControlSection *p_ctr_sec,
                                void *p_data);

/* Process pending control requests (channel creation requests).
 * Create a new channel if a channel candidate is present.
 */
int ivshmem_trusted_control(struct IvshmemControlSection *p_ctr_sec);

/* Create a new channel based on the provided key.
 * Returns 0 on success, -1 on failure.
 */
int ivshmem_create_channel(struct IvshmemControlSection *p_ctr_sec,
                           struct IvshmemChannelKey *p_key);

/* Find an existing channel by key, or create a new one if not found.
 * Returns a pointer to the channel, or NULL on error.
 */
struct IvshmemChannel *ivshmem_find_or_create_channel(
    struct IvshmemControlSection *p_ctr_sec, struct IvshmemChannelKey *key);

/* Initialize a new channel instance with the provided key. */
void ivshmem_init_channel(unsigned int id, struct IvshmemChannel *p_channel,
                          struct IvshmemChannelKey *key);

#endif /* __IVSHMEM_TRUSTED__ */

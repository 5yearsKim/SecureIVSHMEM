#ifndef IVSHMEM_LOCK_BASE_H
#define IVSHMEM_LOCK_BASE_H

#include <pthread.h>
#include <stddef.h>
#include <stdint.h>

#define DATA_SECTION_SIZE \
  (IVSHMEM_SIZE - sizeof(struct IvshmemLockControlSection))
/*
 * IvshmemLockKey: identifies the sender and receiver.
 */
struct IvshmemLockKey {
  uint32_t sender_vm;
  uint32_t sender_pid;
  uint32_t receiver_vm;
};

/*
 * IvshmemLockControlSection: this control structure is stored in shared memory.
 * Immediately following it in memory is the data region (of size
 * CHANNEL_BUF_SIZE). The mutex is process‐shared.
 */
struct IvshmemLockControlSection {
  int ref_count;         /* 0 = no new data, >0 means new data available */
  pthread_mutex_t mutex; /* Process-shared mutex */

  struct IvshmemLockKey key; /* Key for this channel */
  size_t data_size; /* Number of bytes currently stored in the data region */
};

/* Returns a pointer to the data section (immediately after the control
 * structure) */
void *ivshmem_lock_get_data_section(
    struct IvshmemLockControlSection *p_ctr_sec);

/* Initializes the control section (including the process-shared mutex) */
int ivshmem_lock_init_control_section(
    struct IvshmemLockControlSection *p_ctr_sec);

/*
 * ivshmem_lock_send_buffer:
 *
 * Sends an arbitrary-length message from p_buffer (of size 'size' bytes) into
 * the shared data region in fixed-size chunks. For each chunk, the sender:
 *   1. Locks the mutex.
 *   2. Waits until the previous chunk has been consumed (ref_count == 0).
 *   3. Copies the next chunk into the data region.
 *   4. Sets data_size to the chunk size and sets ref_count to 1.
 *   5. Unlocks the mutex.
 *
 * Returns 0 on success.
 */
int ivshmem_lock_send_buffer(struct IvshmemLockKey *p_key,
                             struct IvshmemLockControlSection *p_ctr_sec,
                             void *p_buffer, size_t size);

/*
 * ivshmem_lock_recv_buffer:
 *
 * Receives an arbitrary-length message into p_buffer (of size 'size' bytes) by
 * reading successive chunks from the shared data region. For each chunk, the
 * receiver:
 *   1. Locks the mutex.
 *   2. If ref_count is zero (i.e. no new data), it unlocks and waits.
 *   3. Otherwise, copies the available chunk (data_size bytes) into the proper
 * location in p_buffer.
 *   4. Sets ref_count to 0 (marking it consumed) and unlocks the mutex.
 *
 * The function loops until the full message has been received.
 * Returns 0 on success.
 */
int ivshmem_lock_recv_buffer(struct IvshmemLockKey *p_key,
                             struct IvshmemLockControlSection *p_ctr_sec,
                             void *p_buffer, size_t size);

#endif /* IVSHMEM_LOCK_BASE_H */

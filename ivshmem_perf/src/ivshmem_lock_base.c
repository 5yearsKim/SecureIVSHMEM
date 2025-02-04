#include "ivshmem_lock_base.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ivshmem_lib.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

/*
 * Return a pointer to the data section.
 * We assume that the shared memory region is laid out as:
 *   [ IvshmemLockControlSection ][ data section ]
 */
void *ivshmem_lock_get_data_section(
    struct IvshmemLockControlSection *p_ctr_sec) {
  return (void *)((char *)p_ctr_sec + sizeof(struct IvshmemLockControlSection));
}

/*
 * Initialize the lock control section.
 * Sets up the mutex for process sharing and initializes all other fields.
 */
int ivshmem_lock_init_control_section(
    struct IvshmemLockControlSection *p_ctr_sec) {
  int ret;
  pthread_mutexattr_t attr;

  ret = pthread_mutexattr_init(&attr);
  if (ret != 0) {
    perror("pthread_mutexattr_init");
    return ret;
  }
  ret = pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
  if (ret != 0) {
    perror("pthread_mutexattr_setpshared");
    pthread_mutexattr_destroy(&attr);
    return ret;
  }
  ret = pthread_mutex_init(&p_ctr_sec->mutex, &attr);
  if (ret != 0) {
    perror("pthread_mutex_init");
    pthread_mutexattr_destroy(&attr);
    return ret;
  }
  pthread_mutexattr_destroy(&attr);

  /* Initialize other fields */
  p_ctr_sec->key.sender_vm = 0;
  p_ctr_sec->key.sender_pid = 0;
  p_ctr_sec->key.receiver_vm = 0;
  p_ctr_sec->data_size = 0;
  p_ctr_sec->ref_count = 0;

  return 0;
}

/*
 * Lock-based send:
 * 1. Lock the mutex.
 * 2. Copy 'size' bytes from p_buffer to the data section.
 * 3. Update data_size and increment ref_count (to signal new data).
 * 4. Unlock the mutex.
 */
int ivshmem_lock_send_buffer(struct IvshmemLockKey *p_key,
                             struct IvshmemLockControlSection *p_ctr_sec,
                             void *p_buffer, size_t size) {
  size_t data_sent = 0;
  void *p_data = ivshmem_lock_get_data_section(p_ctr_sec);

  while (1) {
    if ((p_ctr_sec->key.sender_pid != p_key->sender_pid) ||
        (p_ctr_sec->ref_count > 0)) {
      continue;
    }
    if (data_sent >= size) {
      break;
    }

    size_t to_sent = MIN(size - data_sent, DATA_SECTION_SIZE);

    int ret = pthread_mutex_lock(&p_ctr_sec->mutex);
    if (ret != 0) {
      perror("pthread_mutex_lock");
      return ret;
    }

    memcpy(p_data, (char *)p_buffer + data_sent, to_sent);
    data_sent += to_sent;

    p_ctr_sec->key.sender_pid = p_key->sender_pid;
    p_ctr_sec->key.receiver_vm = p_key->receiver_vm;
    p_ctr_sec->key.sender_vm = p_key->sender_vm;
    p_ctr_sec->data_size = to_sent;
    p_ctr_sec->ref_count = 1;

    ret = pthread_mutex_unlock(&p_ctr_sec->mutex);
    if (ret != 0) {
      perror("pthread_mutex_unlock");
      return ret;
    }
  }

  return 0;
}

/*
 * Lock-based receive:
 * 1. Lock the mutex.
 * 2. If ref_count is zero (i.e. no new message), unlock and return -1.
 * 3. Otherwise, copy 'size' bytes from the data section into p_buffer.
 * 4. Decrement ref_count.
 * 5. Unlock the mutex.
 */
int ivshmem_lock_recv_buffer(struct IvshmemLockKey *p_key,
                             struct IvshmemLockControlSection *p_ctr,
                             void *p_buffer, size_t size) {
  void *p_data = ivshmem_lock_get_data_section(p_ctr);
  int data_received = 0;

  while (1) {
    if ((p_ctr->key.receiver_vm != p_key->receiver_vm) ||
        p_ctr->ref_count <= 0) {
      continue;
    }

    if (data_received >= size) {
      break;
    }

    int to_receive = MIN(size - data_received, DATA_SECTION_SIZE);

    int ret = pthread_mutex_lock(&p_ctr->mutex);
    if (ret != 0) {
      perror("pthread_mutex_lock");
      return ret;
    }

    memcpy((char*)p_buffer +data_received, p_data , to_receive);
    p_ctr->ref_count--;  // consume the message
    data_received += to_receive;

    ret = pthread_mutex_unlock(&p_ctr->mutex);
    if (ret != 0) {
      perror("pthread_mutex_unlock");
      return ret;
    }
  }
  return 0;
}

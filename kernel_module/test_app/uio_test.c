#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef HOOK_DEV
#define HOOK_DEV 2 // 1: UIO, 2: MYSHM
#endif

#if   HOOK_DEV == 1
  #define HOOK_DNAME   "uio"
  #define HOOK_TARGET "/dev/uio0"
#elif HOOK_DEV == 2
  #define HOOK_DNAME   "myshm"
  #define HOOK_TARGET "/dev/myshm"
#else
  #error "Unknown HOOK_MODE, must be UIO or MYSHM"
#endif


/* Total 2M shared memory */
/* 0x0000 0000 ~ 0x0000 0fff: control section */
/* 0x0000 1000 ~ 0x0020 0000: data section */

#define CONTROL_SECTION_SIZE 0x1000
#define DATA_SECTION_SIZE 0x1FF000

#define INVALID_LENGTH
// #define SIZE_EXCEEDS
// #define OVERFLOW_LENGTH
#define SUCCESS_MMAP

int main(void) {
        int fd = open(HOOK_TARGET, O_RDWR | O_SYNC);
        if (fd < 0) {
                fprintf(stderr, "[IVSHMEM ERR] Failed to open!\n");
                return -EPERM;
        }

        volatile void *target_addr =
#ifdef INVALID_LENGTH
            /* invalid length value */
            mmap(NULL, 0x2000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
#endif

#ifdef SIZE_EXCEEDS
        /* mapping size exceeds allowd limit */
        mmap(NULL, DATA_SECTION_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
             0x2000);
#endif

#ifdef OVERFLOW_LENGTH
        /* overflow the length value */
        mmap(NULL, DATA_SECTION_SIZE - 0x2000, PROT_READ | PROT_WRITE,
             MAP_SHARED, fd, 0x1000);
#endif

#ifdef SUCCESS_MMAP
        /* success mmap */
        mmap(NULL, CONTROL_SECTION_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
             0);
#endif

        if (target_addr == MAP_FAILED) {
                fprintf(stderr, "[IVSHMEM ERR] Failed to mmap!\n");
        } else {
                fprintf(stdout, "[IVSHMEM] mmap.\n");
                munmap((void *)target_addr, 0x1000);
                fprintf(stdout, "[IVSHMEM] unmap.\n");
        }

        close(fd);

        return 0;
}

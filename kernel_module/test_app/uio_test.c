#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define UIO_DEV "/dev/uio0"

/* Total 2M shared memory */
/* 0x0000 0000 ~ 0x0000 0fff: control section */
/* 0x0000 1000 ~ 0x0020 0000: data section */

#define CONTROL_SECTION_SIZE 0x1000
#define DATA_SECTION_SIZE 0x1FF000

//#define INVALID_LENGTH
//#define SIZE_EXCEEDS
//#define OVERFLOW_LENGTH
#define SUCCESS_MMAP

int main(void) {
        int fd = open(UIO_DEV, O_RDWR | O_SYNC);
        if (fd < 0) {
                fprintf(stderr, "[IAPG ERR] Failed to open!\n");
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
                fprintf(stderr, "[IAPG ERR] Failed to mmap!\n");
        } else {
                fprintf(stdout, "[IAPG] mmap.\n");
                munmap((void *)target_addr, 0x1000);
                fprintf(stdout, "[IAPG] unmap.\n");
        }

        close(fd);

        return 0;
}

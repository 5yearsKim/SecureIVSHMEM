// test_shmem.c
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#define SIZE (4 * 1024 * 1024)  // Map the full 4MB

int main(void)
{
    int fd = open("/dev/myshm", O_RDWR);
    if (fd < 0) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Map 4MB from /dev/myshm into our address space.
    void *addr = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }
    
    // Write a test value into the mapped region.
    uint32_t *p = (uint32_t *)addr;
    p[0] = 0xDEADBEEF;
    
    // Read it back and print.
    printf("Value at start: 0x%X\n", p[0]);

    // Clean up.
    munmap(addr, SIZE);
    close(fd);
    return 0;
}

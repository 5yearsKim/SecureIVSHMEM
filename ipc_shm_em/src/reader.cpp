#include <iostream>
#include <cstring>      // for strlen
#include <fcntl.h>      // for O_* constants
#include <sys/mman.h>   // for shm_open, mmap
#include <sys/stat.h>   // for fstat, mode constants
#include <unistd.h>     // for close, munmap, shm_unlink

int main() {
    // Name must match the writer's name
    const char* shmName = "/mysharedmem";
    const size_t SIZE = 4096;

    // 1. Open the existing shared memory object (created by writer)
    int shmFd = shm_open(shmName, O_RDONLY, 0666);
    if (shmFd == -1) {
        std::cerr << "Failed to open shared memory\n";
        return 1;
    }

    // 2. Map the shared memory for reading
    void* ptr = mmap(nullptr, SIZE, PROT_READ, MAP_SHARED, shmFd, 0);
    if (ptr == MAP_FAILED) {
        std::cerr << "Failed to map shared memory\n";
        close(shmFd);
        return 1;
    }

    // 3. Read the message
    std::cout << "Reader received message: " 
              << static_cast<char*>(ptr) << std::endl;

    // 4. Unmap the memory and close the file descriptor
    munmap(ptr, SIZE);
    close(shmFd);

    // 5. Optionally, remove (unlink) the shared memory object
    //    so it doesn't remain in the system. 
    //    (If you want multiple readers, you might delay or skip this.)
    if (shm_unlink(shmName) == -1) {
        std::cerr << "Failed to unlink shared memory\n";
    }

    std::cout << "Reader done.\n";
    return 0;
}

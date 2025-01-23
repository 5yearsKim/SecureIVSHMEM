#include <iostream>
#include <cstring>      // for strcpy, strlen
#include <fcntl.h>      // for O_* constants
#include <sys/mman.h>   // for shm_open, mmap
#include <sys/stat.h>   // for fstat, mode constants
#include <unistd.h>     // for ftruncate, close

int main() {
    // Name for the shared memory object
    const char* shmName = "/mysharedmem";
    // Size of the shared memory (in bytes)
    const size_t SIZE = 4096;

    // 1. Create (or open) a shared memory object
    //    O_CREAT  : Create the shared memory if it doesn’t exist
    //    O_RDWR   : Open for read-write
    //    0666     : Permissions (read/write for owner, group, others)
    int shmFd = shm_open(shmName, O_CREAT | O_RDWR, 0666);
    if (shmFd == -1) {
        std::cerr << "Failed to create/open shared memory\n";
        return 1;
    }

    // 2. Set the size of the shared memory object
    if (ftruncate(shmFd, SIZE) == -1) {
        std::cerr << "Failed to set size for shared memory\n";
        return 1;
    }

    // 3. Map the shared memory into the process address space
    void* ptr = mmap(nullptr, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0);
    if (ptr == MAP_FAILED) {
        std::cerr << "Failed to map shared memory\n";
        return 1;
    }

    // 4. Write a message to the shared memory
    const char* message = "Hello from the writer process!";
    strcpy(static_cast<char*>(ptr), message);

    std::cout << "Writer wrote message: " << message << std::endl;

    // 5. Unmap and close (but do NOT unlink yet, so the reader can read)
    if (munmap(ptr, SIZE) == -1) {
        std::cerr << "Failed to unmap\n";
    }
    close(shmFd);

    std::cout << "Writer done.\n";
    return 0;
}

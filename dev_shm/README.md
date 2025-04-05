# myshm - Kernel Module for Shared Memory on /dev

## Overview

`myshm` is a simple Linux kernel character device driver that allocates a 4MB shared memory buffer under `/dev/myshm` and supports `mmap`. It emulates an IVSHMEM-like feature on a local folder, allowing user-space applications to map and access shared memory via a character device.

## Features

- Allocates 4MB of memory in the kernel with `vmalloc`.
- Exposes a character device `/dev/myshm` for user-space interaction.
- Supports `mmap` for zero-copy shared memory access.
- Simple open and close operations with kernel log notifications.

## Prerequisites

- Linux kernel headers and development packages installed.
- GCC and Make build tools.
- `sudo` privileges to load/unload kernel modules.

## Directory Structure


## Build and Load

The `build_and_load.sh` script automates building and inserting the module:

```shell
./build_and_load.sh
```

Alternatively, you can build and insert manually:

```shell
make
sudo insmod myshm.ko
```

After loading, the device node `/dev/myshm` will be created automatically.

## Usage

1. Open the device in your user-space program:

   ```c
   int fd = open("/dev/myshm", O_RDWR);
   if (fd < 0) { perror("open"); exit(1); }
   ```

2. Map the memory:

   ```c
   void *ptr = mmap(NULL, 4 * 1024 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
   if (ptr == MAP_FAILED) { perror("mmap"); exit(1); }
   ```

3. Read from and write to `ptr` as needed.

4. Unmap and close:

   ```c
   munmap(ptr, 4 * 1024 * 1024);
   close(fd);
   ```

## Unloading the Module

To remove the module from the kernel:

```shell
sudo rmmod myshm
```

## Cleaning Up

Remove build artifacts with:

```shell
make clean
```

## Examples

The `examples/` directory contains `write_test.c`, a sample program that opens `/dev/myshm`, maps the buffer, writes a pattern, and reads it back to verify functionality.

To build and run the example:

```shell
cd examples
gcc write_test.c -o write_test
./write_test
```

## Troubleshooting

- Check `dmesg` for kernel log messages prefixed with `myshm:`.
- Ensure you have sufficient permissions to access `/dev/myshm`.

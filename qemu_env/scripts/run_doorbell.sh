#!/bin/bash

qemu-system-x86_64 \
  -m 1024M \
  -drive file=./images/alpine_disk.qcow2,if=virtio,format=qcow2 \
  -boot c \
  -enable-kvm \
  -smp 2 \
  -net nic \
  -net user \
  -nographic \
  -object memory-backend-file,size=1M,share=on,mem-path=/dev/shm/ivshmem,id=hostmem \
  -device ivshmem-doorbell,vectors=1,chardev=ivshmem \
  -chardev socket,id=ivshmem,path=/tmp/ivshmem_socket \
  -virtfs local,path=./shared,security_model=passthrough,mount_tag=shared \
  -name "AlpineTestVM"

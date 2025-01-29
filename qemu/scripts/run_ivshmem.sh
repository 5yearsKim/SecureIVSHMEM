#!/bin/sh

# qemu-system-x86_64 \
#   -m 4096M \
#   -drive file=./images/ubuntu_disk.qcow2,if=virtio \
#   -boot c \
#   -enable-kvm \
#   -smp 4 \
#   -net nic \
#   -net user \
#   -display gtk \
#   -vga virtio \
#   -object memory-backend-file,size=1M,share=on,mem-path=/dev/shm/ivshmem,id=hostmem \
#   -device ivshmem-plain,memdev=hostmem \
#   -name "UbuntuTestVM"

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
  -device ivshmem-plain,memdev=hostmem \
  -virtfs local,path=./shared,security_model=passthrough,mount_tag=shared \
  -name "AlpineTestVM"




## Additional options
#  -cdrom ./images/alpine-standard-3.21.2-x86_64.iso \
#  -snapshot : Runs the VM in snapshot mode, where changes are not written to the disk image.

## temp options
#  -device ivshmem-doorbell,vectors=4,chardev=ivshmem1 \
#  -chardev socket,path=/tmp/ivshmem_socket,id=ivshmem1 \

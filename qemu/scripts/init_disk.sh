#!/bin/bash

qemu-system-x86_64 \
  -m 4096\
  -smp 4 \
  -enable-kvm \
  -cdrom ./images/ubuntu-24.04.1-live-server-amd64.iso \
  -hda ./images/ubuntu_disk.qcow2 \
  -boot d \
  -net nic \
  -net user \
  -display gtk \
  -vga virtio
  -name "Ubuntu_Test_VM"



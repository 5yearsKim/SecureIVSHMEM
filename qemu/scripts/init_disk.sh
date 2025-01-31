#!/bin/bash

qemu-system-x86_64 \
  -m 4096\
  -smp 4 \
  -enable-kvm \
  -cdrom ./images/alpine-standard-3.21.2-x86_64.iso \
  -hda ./images/alpine_disk.qcow2 \
  -boot d \
  -net nic \
  -net user \
  -display gtk \
  -vga virtio
  -name "Ubuntu_Test_VM"



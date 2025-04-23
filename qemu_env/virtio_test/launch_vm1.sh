#!/bin/bash


qemu-system-x86_64 \
  -enable-kvm -m 2G \
  -drive file=../images/alpine_disk.qcow2,if=virtio,cache=none \
  -netdev socket,id=net0,listen=127.0.0.1:6000 \
  -device virtio-net-pci,netdev=net0 \
  -nographic

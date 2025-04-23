#!/bin/bash


# Connector side:
qemu-system-x86_64 \
  -enable-kvm -m 2G \
  -drive file=../images/alpine_disk2.qcow2,if=virtio,cache=none \
  -netdev socket,id=net1,connect=127.0.0.1:6000 \
  -device virtio-net-pci,netdev=net1 \
  -nographic

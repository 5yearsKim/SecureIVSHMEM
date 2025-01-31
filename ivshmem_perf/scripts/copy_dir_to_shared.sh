#!/bin/bash

# Copy a directory to the shared memory

WORK_DIR=$(realpath $(dirname $0)/..)
TARET_DIR="../qemu/shared/ivshmem_perf"

cd $WORK_DIR

mkdir -p "$TARET_DIR"

cp -r ./* "$TARET_DIR"

echo "Copied all files to $TARET_DIR"

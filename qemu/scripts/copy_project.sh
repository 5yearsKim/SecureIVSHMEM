#!/bin/bash


PRJ_NAME=ipc_shm_em
IS_ROOT=true
HOME="/root"

# if root -> "", else "sudo "
SUDO_PREFIX=$(if [ $IS_ROOT = "true" ]; then echo ""; else echo "sudo "; fi)



# Copy the project to the qemu directory

# Create a shared directory if not created
${SUDO_PREFIX} mkdir -p /mnt/shared
$(SUDO_PREFIX) mount -t 9p -o trans=virtio,version=9p2000.L shared /mnt/shared

# exit process if the project directory does not exist
if [ ! -d /mnt/shared/${PRJ_NAME} ]; then
    echo "Project directory does not exist"
    exit 1
fi

# Copy the project directory from shared memory to home
cp -r /mnt/shared/${PRJ_NAME} ~
echo "Project copied to home directory"

# Change the working directory to the project directory
WORK_DIR="${HOME}/${PRJ_NAME}"
cd ${WORK_DIR}

rm -rf build
mkdir build
cd build
cmake ..
make

echo "Project built successfully"



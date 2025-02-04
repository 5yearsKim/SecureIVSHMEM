#!/bin/bash



# Default values
IMAGE="./images/alpine_disk.qcow2"
TYPE="plain"
NAME="AlpineTestVM"

# Function to display usage information.
usage() {
  echo "Usage: $0 [--image <path>] [--type <doorbell|plain>] [--name <name>]"
  exit 1
}

# Parse command-line options using getopt.
# The options string is empty because we're only using long options.
PARSED_ARGS=$(getopt --options="" --long image:,type:,name: -- "$@")
if [ $? -ne 0 ]; then
  usage
fi

# The 'eval' along with 'set' prepares the positional parameters.
eval set -- "$PARSED_ARGS"

# Process options.
while true; do
  case "$1" in
    --image)
      echo "image set as $2"
      IMAGE="$2"
      shift 2
      ;;
    --type)
      type="$2"
      # Validate type: only allow "doorbell" or "plain"
      if [[ "$type" != "doorbell" && "$type" != "plain" ]]; then
        echo "Error: Invalid type '$type'. Allowed values are 'doorbell' or 'plain'."
        usage
      fi
      shift 2
      ;;
    --name)
      NAME="$2"
      shift 2
      ;;
    --)
      shift
      break
      ;;
    *)
      echo "Internal error!"
      exit 1
      ;;
  esac
done


# Set options

SHM_OPT=""
if [ "$TYPE" == "doorbell" ]; then
  SHM_OPT="-device ivshmem-doorbell,vectors=1,chardev=ivshmem -chardev socket,id=ivshmem,path=/tmp/ivshmem_socket"
elif [ "$TYPE" == "plain" ]; then
  SHM_OPT="-device ivshmem-plain,memdev=hostmem"
fi

qemu-system-x86_64 \
  -m 1024M \
  -drive file=${IMAGE},if=virtio,format=qcow2 \
  -boot c \
  -enable-kvm \
  -smp 2 \
  -net nic \
  -net user \
  -nographic \
  -object memory-backend-file,size=4M,share=on,mem-path=/dev/shm/ivshmem,id=hostmem \
  $SHM_OPT \
  -virtfs local,path=./shared,security_model=passthrough,mount_tag=shared \
  -name $NAME




## Additional options
#  -cdrom ./images/alpine-standard-3.21.2-x86_64.iso \
#  -snapshot : Runs the VM in snapshot mode, where changes are not written to the disk image.

## temp options
#  -device ivshmem-doorbell,vectors=4,chardev=ivshmem1 \
#  -chardev socket,path=/tmp/ivshmem_socket,id=ivshmem1 \

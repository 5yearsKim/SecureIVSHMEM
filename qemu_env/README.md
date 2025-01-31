# QEMU environment setup

## QEMU Setup

### Create a QCOW2 disk image:
```sh
qemu-img create -f qcow2 alpine_disk.qcow2 8G
```

Boot image from `iso` file with following command:

```sh
qemu-system-x86_64 \
  -m 1024M \
  -cdrom ./images/alpine-standard-3.21.2-x86_64.iso \
  -hda ./images/alpine_disk.qcow2 \
  -boot d \
  -enable-kvm \
  -smp 2 \
  -net nic \
  -net user \
  -nographic \
  -name "Alpine_Test_VM"
```

login with id `root` and type default password (just enter) and login. Then install alpine image with:

```sh
setup-alpine
```

> Note that you should select `Disk` as `sda`. Select `sys` after then.

### References
- [QEMU install alpine linux](https://daehee87.tistory.com/182)

<br/>

#### VM Setup

For setting up VM environment, follow this guide:
<br/>
[Setup Alpine Linux](./docs/setup_alpine.md)



### Make a Shared Directory

make a dirctory `shared` and use this option:

```sh
  -virtfs local,path=./shared,security_model=passthrough,mount_tag=shared \
```

then type this command on guest VM,

```sh
sudo mkdir -p /mnt/shared
sudo mount -t 9p -o trans=virtio,version=9p2000.L shared /mnt/shared
```

- `-t 9p`: Specifies the 9p filesystem type.
- `-o trans=virtio,version=9p2000.L`: Sets the transport method to virtio and specifies the 9p protocol version.
- `shared`: The mount_tag specified in the QEMU command.
- `/mnt/shared`: The mount point inside the guest.


## Interrupt Server Setup

```sh
sudo ./ivshmem-server -v -F -S /tmp/ivshmem_socket -M ivshmem -m /dev/shm -l 1M -n 1
```



## IVSHMEM Communication Tutorial

https://liujunming.top/2021/11/30/QEMU-tutorial-Inter-VM-Shared-Memory-device/






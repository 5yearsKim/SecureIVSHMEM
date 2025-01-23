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

[references](https://daehee87.tistory.com/182)







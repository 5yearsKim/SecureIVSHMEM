# Setup Alpine Linux


## Install Dependancies

Install following packages:
```sh
apk update
apk add bash vim \
    pciutils \ # for lspci
    build-base \ # for building C code
    cmake \
    make \
    g++ \

```
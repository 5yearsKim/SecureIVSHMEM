#!/bin/bash

echo ">> Building and loading ivshm module";
make clean;make;

if [ $? -ne 0 ]; then
    echo ">> Failed to build myshm module";
    exit 1;
fi

sudo lsmod | grep myshm;

# rmmod if already loaded
if [ $? -eq 0 ]; then
    echo ">> Unloading myshm module";
    sudo rmmod myshm;
fi

echo ">> Loading myshm module";
sudo insmod myshm.ko;

if [ $? -ne 0 ]; then
    echo ">> Failed to load myshm module";
    exit 1;
fi


sudo chmod 666 /dev/myshm;

if [ $? -ne 0 ]; then
    echo ">> Failed to change permission of /dev/myshm";
    exit 1;
fi

echo ">> Done";
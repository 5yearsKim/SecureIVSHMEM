#!/bin/bash

NAME="myshm"

echo ">> Building and loading $NAME module";
make clean;make;

if [ $? -ne 0 ]; then
    echo ">> Failed to build $NAME module";
    exit 1;
fi

sudo lsmod | grep $NAME;

# rmmod if already loaded
if [ $? -eq 0 ]; then
    echo ">> Unloading $NAME module";
    sudo rmmod $NAME;
fi

echo ">> Loading $NAME module";
sudo insmod output/$NAME.ko;

if [ $? -ne 0 ]; then
    echo ">> Failed to load $NAME module";
    exit 1;
fi


sudo chmod 666 /dev/$NAME;

if [ $? -ne 0 ]; then
    echo ">> Failed to change permission of /dev/$NAME";
    exit 1;
fi

echo ">> Done";
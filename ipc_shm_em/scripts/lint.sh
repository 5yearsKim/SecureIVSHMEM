#!/bin/bash

SRC_DIR="./src"

# File list to be inspected.
files=`find ${SRC_DIR} -type f \
    \( -name *.h -o -name *.hpp -o -name *.cpp -o -name *.c \) \
    ! -path "${SRC_DIR}/libb64/*"`

echo "Running clang-format on the following files: ${files}"


clang-format -i -style=Google ${files}
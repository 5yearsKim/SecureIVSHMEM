#!/bin/bash

# Make a script to generate a prompt for ChatGPT. I want all files under ./src to be printed with file paths

# Get all files under ./src
# files=$(find ./src -type f)
files=("./src/reader.c" "./src/ivshmem_lib.h" "./src/ivshmem_lib.c")


for file in ${files[@]} ; do
    echo "====================="
    echo "File: $file'"
    echo "====================="
    echo "$(cat $file)"
    echo "===========end of file "${file}" ========="
done

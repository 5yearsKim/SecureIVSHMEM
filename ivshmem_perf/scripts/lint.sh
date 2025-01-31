#!/bin/bash

SRC_DIR="./src"
INCLUDE_DIR="./include"

# Find files in both src and include directories
files=$(find "${SRC_DIR}" "${INCLUDE_DIR}" -type f \
    \( -name "*.h" -o -name "*.hpp" -o -name "*.cpp" -o -name "*.c" \) \
    ! -path "${SRC_DIR}/libb64/*") # Exclude libb64 in src


echo "Running clang-format on the following files: ${files}"


clang-format -i -style=Google ${files}
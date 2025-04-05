#!/bin/bash

# Get all files under ./src, ./include, and ./examples
files=$(find ./src ./include ./examples -type f)

# files=(
#     "./src/writer.c"
#     "./include/ivshmem_lib.h"
#     "./src/ivshmem_lib.c"
# )

# Build the prompt string
prompt=""

for file in ${files[@]}; do
    prompt+="==========File: $file===========\n"
    prompt+="$(cat "$file")\n"  # Double quotes around $file are crucial
    prompt+="===========end of file \"$file\" =========\n"  # Escaping the double quote inside the string
done

echo "$prompt" | xclip -selection clipboard

echo "Prompt generated and copied to clipboard."

exit 0
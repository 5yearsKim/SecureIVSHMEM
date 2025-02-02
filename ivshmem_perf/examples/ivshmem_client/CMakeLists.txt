cmake_minimum_required(VERSION 3.10)
project(ivshmem-client C)

set(CMAKE_C_STANDARD 99)

# Optionally, add compile definitions for _GNU_SOURCE if needed.
add_definitions(-D_GNU_SOURCE)

add_executable(ivshmem-client
    main.c
    ivshmem-client.c
    )

# If you need to link with pthread or other libraries, do so here.
# target_link_libraries(ivshmem-client pthread)

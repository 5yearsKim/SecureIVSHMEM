
cmake_minimum_required(VERSION 3.10)
project(reader C)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

set(IVSHMEM_LIB_SOURCES
    ${CMAKE_SOURCE_DIR}/src/ivshmem_lib.c
    ${CMAKE_SOURCE_DIR}/src/ivshmem_data.c
)

add_executable(reader
    main.c
    ${IVSHMEM_LIB_SOURCES}
)

set(IVSHMEM_LOCK_LIB_SOURCES
    ${CMAKE_SOURCE_DIR}/src/ivshmem_lib.c
    ${CMAKE_SOURCE_DIR}/src/ivshmem_lock_base.c
)

add_executable(lock_reader
    lock_base_main.c
    ${IVSHMEM_LOCK_LIB_SOURCES}
)

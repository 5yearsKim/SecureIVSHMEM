
cmake_minimum_required(VERSION 3.10)
project(no_lock_test C)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

set(IVSHMEM_LIB_SOURCES
    ${CMAKE_SOURCE_DIR}/src/ivshmem_lib.c
    ${CMAKE_SOURCE_DIR}/src/ivshmem_secure.c
)

add_executable(no_lock_writer
    writer.c
    ${IVSHMEM_LIB_SOURCES}
)


add_executable(no_lock_reader
    reader.c
    ${IVSHMEM_LIB_SOURCES}
)

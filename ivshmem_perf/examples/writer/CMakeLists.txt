
cmake_minimum_required(VERSION 3.10)
project(writer C)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

set(IVSHMEM_LIB_SOURCES
    ${CMAKE_SOURCE_DIR}/src/ivshmem_lib.c
    ${CMAKE_SOURCE_DIR}/src/ivshmem_data.c
)

add_executable(writer
    main.c
    ${IVSHMEM_LIB_SOURCES}
)


add_executable(lock_writer
    lock_base_main.c
    ${CMAKE_SOURCE_DIR}/src/ivshmem_lib.c
    ${CMAKE_SOURCE_DIR}/src/ivshmem_lock_base.c
)

add_executable(lock_init
    lock_init.c
    ${CMAKE_SOURCE_DIR}/src/ivshmem_lib.c
    ${CMAKE_SOURCE_DIR}/src/ivshmem_lock_base.c

)

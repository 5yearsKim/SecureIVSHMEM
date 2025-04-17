cmake_minimum_required(VERSION 3.10)
project(IVSHMEM_Examples C)


add_subdirectory(
  ${CMAKE_CURRENT_LIST_DIR}/../secure_ivshmem   
  ${CMAKE_BINARY_DIR}/secure_ivshmem_build
)

add_subdirectory(no_lock_test)
add_subdirectory(reader)
add_subdirectory(trusted_main)
add_subdirectory(writer)

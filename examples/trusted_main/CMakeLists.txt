cmake_minimum_required(VERSION 3.10)
project(trusted_main C)

# add_subdirectory(
#   ${CMAKE_CURRENT_LIST_DIR}/../../secure_ivshmem 
#   ${CMAKE_BINARY_DIR}/secure_ivshmem_build 
# )

add_executable(trusted_main main.c)
target_link_libraries(trusted_main PRIVATE secure_ivshmem)

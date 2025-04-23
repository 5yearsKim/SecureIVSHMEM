cmake_minimum_required(VERSION 3.10)
project(writer C)

# add_subdirectory(
#   ${CMAKE_CURRENT_LIST_DIR}/../../secure_ivshmem 
#   ${CMAKE_BINARY_DIR}/secure_ivshmem_build 
# )

add_executable(writer main.c)
target_link_libraries(writer PRIVATE secure_ivshmem)

add_executable(lock_writer lock_base_main.c)
target_link_libraries(lock_writer PRIVATE secure_ivshmem)

add_executable(lock_init lock_init.c)
target_link_libraries(lock_init PRIVATE secure_ivshmem)


find_package(OpenSSL REQUIRED)

add_executable(aes_writer aes_main.c)
target_link_libraries(aes_writer PRIVATE secure_ivshmem OpenSSL::Crypto)
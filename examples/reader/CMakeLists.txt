cmake_minimum_required(VERSION 3.10)
project(reader C)

# add_subdirectory(
#   ${CMAKE_CURRENT_LIST_DIR}/../../secure_ivshmem 
#   ${CMAKE_BINARY_DIR}/secure_ivshmem_build 
# )

add_executable(reader main.c)
target_link_libraries(reader PRIVATE secure_ivshmem)

add_executable(lock_reader lock_base_main.c)
target_link_libraries(lock_reader PRIVATE secure_ivshmem)


find_package(OpenSSL REQUIRED)

add_executable(aes_reader aes_main.c)
target_link_libraries(aes_reader PRIVATE secure_ivshmem OpenSSL::Crypto)
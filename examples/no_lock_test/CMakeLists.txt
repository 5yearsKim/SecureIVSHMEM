
cmake_minimum_required(VERSION 3.10)
project(no_lock_test C)

add_subdirectory(
  ${CMAKE_CURRENT_LIST_DIR}/../../secure_ivshmem    # path to B’s CMakeLists.txt
  ${CMAKE_BINARY_DIR}/secure_ivshmem_build          # out‑of‑tree build dir
)

add_executable(no_lock_writer writer.c)
target_link_libraries(no_lock_writer PRIVATE secure_ivshmem)

add_executable(no_lock_reader reader.c)
target_link_libraries(no_lock_reader PRIVATE secure_ivshmem)


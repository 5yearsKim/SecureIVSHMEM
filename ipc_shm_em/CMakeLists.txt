
cmake_minimum_required(VERSION 3.10)

project(ipc_shm_em)

include_directories(include)


set(READER_SRC
    src/reader.c
    src/ivshmem_lib.c
)

set(WRITER_SRC
    src/writer.c
    src/ivshmem_lib.c
)

add_executable(reader ${READER_SRC})
add_executable(writer ${WRITER_SRC})





project(ipc_shm_em)

set(SRC_FILES
    src/reader.cpp
    src/writer.cpp
)

add_executable(reader src/reader.cpp)
add_executable(writer src/writer.cpp)



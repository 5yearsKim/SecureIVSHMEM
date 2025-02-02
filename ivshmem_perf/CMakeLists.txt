
cmake_minimum_required(VERSION 3.10)

project(ipc_shm_em)

# subdirectory for the ivshmem library

add_subdirectory(examples/ivshmem_client)
# Set the ivshmem-client target's output directory
set_target_properties(ivshmem-client PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/ivshmem_client"
)

add_subdirectory(examples/writer)
set_target_properties(writer PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/writer"
)

add_subdirectory(examples/reader)
set_target_properties(reader PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/reader"
)

# Set the ivshmem-client target's output directory

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)


file(GLOB TRUSTED_MAIN_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/trusted_main.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)
list(FILTER TRUSTED_MAIN_SOURCES EXCLUDE REGEX "_ivshmem_kernel\\.c$")


# Add the trusted_main executable.
add_executable(trusted_main ${TRUSTED_MAIN_SOURCES})



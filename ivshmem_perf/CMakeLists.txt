
cmake_minimum_required(VERSION 3.10)

project(ipc_shm_em)


execute_process(
    COMMAND uname -r
    OUTPUT_VARIABLE KERNEL_RELEASE
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

message(STATUS "Kernel release: ${KERNEL_RELEASE}")

# subdirectory for the ivshmem library
add_subdirectory(examples/ivshmem_client)

# examples/writer
add_subdirectory(examples/writer)
set_target_properties(writer lock_writer lock_init PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/writer"
)

# examples/reader
add_subdirectory(examples/reader)
set_target_properties(reader lock_reader PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/reader"
)

#  examples/no_lock_test
add_subdirectory(examples/no_lock_test)


# Set the ivshmem-client target's output directory

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)


file(GLOB TRUSTED_MAIN_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/trusted_main.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)
# list(FILTER TRUSTED_MAIN_SOURCES EXCLUDE REGEX "hw/*.c$")


# Add the trusted_main executable.
add_executable(trusted_main ${TRUSTED_MAIN_SOURCES})


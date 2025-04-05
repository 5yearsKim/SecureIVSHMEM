
cmake_minimum_required(VERSION 3.10)

project(secure_ivshmem)


execute_process(
    COMMAND uname -r
    OUTPUT_VARIABLE KERNEL_RELEASE
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

message(STATUS "Kernel release: ${KERNEL_RELEASE}")


file(GLOB IVSHMEM_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

file(GLOB IVSHMEM_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)

add_library(secure_ivshmem STATIC
  ${IVSHMEM_SOURCES}
)

target_include_directories(secure_ivshmem
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

set_target_properties(secure_ivshmem PROPERTIES
  VERSION        ${PROJECT_VERSION}
  SOVERSION      ${PROJECT_VERSION_MAJOR}
)



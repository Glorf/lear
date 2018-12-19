cmake_minimum_required(VERSION 3.12)

set(CMAKE_C_STANDARD 11)
set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})
set(LEAR_INSTALL_DIR ${PROJECT_SOURCE_DIR}/bin)
set(RUNTIME_OUTPUT_DIRECTORY ${LEAR_INSTALL_DIR})

add_compile_options(-Wall -std=c11 -O3)

add_subdirectory(src)

cmake_minimum_required(VERSION 3.12)
project(lear C)

set(CMAKE_C_STANDARD 11)

add_compile_options(-Wall)
add_executable(lear main.c master.c master.h worker.c worker.h connection.c connection.h config.c config.h logger.c logger.h http.c http.h cache.c cache.h types.h types.c)
target_link_libraries(lear yaml)
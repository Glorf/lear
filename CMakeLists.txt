cmake_minimum_required(VERSION 3.12)
project(puthttpd C)

set(CMAKE_C_STANDARD 11)

add_executable(puthttpd main.c master.c master.h worker.c worker.h thread.c thread.h connection.c connection.h)
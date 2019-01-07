cmake_minimum_required(VERSION 3.12)
project(lear C)

add_executable(lear main.c master.c master.h worker.c worker.h connection.c connection.h config.c config.h logger.c logger.h http.c http.h cache.c cache.h types.h types.c siphash.h)
target_link_libraries(lear yaml)
install(TARGETS lear DESTINATION ${LEAR_INSTALL_DIR})

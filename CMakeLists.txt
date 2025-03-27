cmake_minimum_required(VERSION 3.28)

include_directories(include)
link_directories()

file(GLOB_RECURSE SOURCES src/*.c)
add_executable(akcc ${SOURCES})
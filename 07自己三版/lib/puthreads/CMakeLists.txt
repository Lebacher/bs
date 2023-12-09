cmake_minimum_required(VERSION 3.0.0)
project(Puthreads VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_FLAGS "-std=c11 -Wall -g -Wconversion -Wsign-conversion -O0 -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE ${CMAKE_C_FLAGS}")

file(GLOB SOURCES "*.c")
file(GLOB HEADERS "*.h")

add_library(puthreads ${SOURCES} ${HEADERS})


cmake_minimum_required(VERSION 3.0.0)

project(07R_User_Scheduling VERSION 0.1.0 LANGUAGES C)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)
set(CMAKE_C_FLAGS "-std=c11 -Wall -g -Wconversion -Wsign-conversion -O0 -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE")

file(GLOB SOURCES "src/*.c")

add_subdirectory(lib/puthreads)

add_executable(scheduler ${SOURCES})
target_link_libraries(scheduler puthreads)

include_directories(${Puthreads_SOURCE_DIR})
include_directories(${07R_User_Scheduling_SOURCE_DIR}/include)


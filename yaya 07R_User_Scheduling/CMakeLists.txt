set(CMAKE_C_COMPILER "/usr/bin/clang")

cmake_minimum_required(VERSION 3.0.0)

project(07R_User_Scheduling VERSION 0.1.0 LANGUAGES C)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

# used instead of target_link_options because it is unavailable below version 3.13
set(CMAKE_EXE_LINKER_FLAGS
  "-fcolor-diagnostics -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls")

# build puthreads library
add_library(puthreads ${CMAKE_SOURCE_DIR}/lib/puthreads)
add_subdirectory(lib/puthreads)

# build scheduler executable
add_executable(scheduler ${CMAKE_SOURCE_DIR}/src/main.c ${CMAKE_SOURCE_DIR}/src/demo.c ${CMAKE_SOURCE_DIR}/src/sem.c)
add_subdirectory(src)

# link scheduler executable against puthreads lib
target_link_libraries(scheduler
  puthreads)

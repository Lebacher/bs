cmake_minimum_required(VERSION 3.0)

set(CMAKE_C_COMPILER "/usr/bin/clang")

project(myGroup-server C)

#Only one sanitizer may be enabled at any time
set(sanitizeThread  ON)
set(sanitizeAddress OFF)

#the cmake way to activate a well known library (here pthreads)
find_package (Threads)

add_executable(myGroup-server ${CMAKE_SOURCE_DIR}/src/main.c)

target_sources(myGroup-server
  PRIVATE
  src/main.c
  src/reads_list.c
  src/ring_buffer.c
  src/server.c)

target_include_directories(myGroup-server
  PRIVATE
  include)

target_compile_options(myGroup-server
  PRIVATE
  -std=c11
  -O0
  -g
  -pedantic
  -Wall
  -Wextra
  -Werror
  -Wno-unused-function
  -fcolor-diagnostics
  -fno-omit-frame-pointer
  -fno-optimize-sibling-calls)

target_compile_definitions(myGroup-server
  PRIVATE
  _XOPEN_SOURCE
  _XOPEN_SOURCE_EXTENDED
  _GNU_SOURCE)

#Enable Clang Debug Tool Support, only one of this two will work at the same time
if(sanitizeThread)
    # -fsanitize=thread enables the clang thread sanitizer
    # see http://clang.llvm.org/docs/ThreadSanitizer.html for more details
    target_compile_options(myGroup-server
      PRIVATE
      -fsanitize=thread
      -fPIE)

    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread")
endif()
if(sanitizeAddress)
    # -fsanitize=address enables the clang address sanatizer
    # see http://clang.llvm.org/docs/AddressSanitizer.html for more details
    target_compile_options(myGroup-server
      PRIVATE
      -fsanitize=address)

    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
endif()

#add dynamic linked libraries
target_link_libraries (myGroup-server ${CMAKE_THREAD_LIBS_INIT})

#chmod +x binaries -- just to be safe
add_custom_target(makeExecutable ALL
    COMMAND chmod +x ${CMAKE_CURRENT_SOURCE_DIR}/ressources/myGroup-*
)

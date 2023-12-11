cmake_minimum_required(VERSION 3.0)

set(CMAKE_C_COMPILER "/usr/bin/clang")

project(myGroup-client C)

#Only one sanitizer may be enabled at any time
set(sanitizeThread  OFF)
set(sanitizeAddress ON)

#the cmake way to activate a well known library (here pthreads)
find_package (Threads)

add_executable(myGroup-client ${CMAKE_SOURCE_DIR}/src/main.c)

target_sources(myGroup-client
  PRIVATE
  src/main.c
  src/utils.c
  src/client.c)

target_include_directories(myGroup-client
  PRIVATE
  include)

target_compile_options(myGroup-client
  PRIVATE
  -std=c11
  -O1
  -g
  -pedantic
  -Wall
  -Wextra
  -Werror
  -Wno-unused-function
  -fcolor-diagnostics
  -fno-omit-frame-pointer
  -fno-optimize-sibling-calls)

target_compile_definitions(myGroup-client
  PRIVATE
  _XOPEN_SOURCE
  _XOPEN_SOURCE_EXTENDED
  _GNU_SOURCE)

#Enable Clang Debug Tool Support, only one of this two will work at the same time
if(sanitizeThread)
    # -fsanitize=thread enables the clang thread sanitizer
    # see http://clang.llvm.org/docs/ThreadSanitizer.html for more details
    target_compile_options(myGroup-client
      PRIVATE
      -fsanitize=thread
      -fPIE)

    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread")
endif()
if(sanitizeAddress)
    # -fsanitize=address enables the clang address sanatizer
    # see http://clang.llvm.org/docs/AddressSanitizer.html for more details
    target_compile_options(myGroup-client
      PRIVATE
      -fsanitize=address)

    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
endif()

#add dynamic linked libraries
target_link_libraries (myGroup-client ${CMAKE_THREAD_LIBS_INIT})

# collect all ressources
file(GLOB RESSOURCES ressources/*)

# list ressources in IDE
add_custom_target(listRessources SOURCES
    ${RESSOURCES}
)

# copy ressources into build folder
add_custom_target(deployRessources ALL
    COMMAND cp ${RESSOURCES} ${CMAKE_BINARY_DIR}
	COMMAND chmod +x ${CMAKE_BINARY_DIR}/myGroup-server
	COMMAND chmod +x ${CMAKE_BINARY_DIR}/run_tests.bash
)

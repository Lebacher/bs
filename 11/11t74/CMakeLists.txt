#define the minimum required cmake version to execute this script (this must be provided in every CMakeLists.txt)
cmake_minimum_required(VERSION 2.8)

#name of the project
project(myGroup-server C)

#Only one sanitizer may be enabled at any time
set(sanitizeThread  ON)
set(sanitizeAddress OFF)

#the cmake way to activate a well known library (here pthreads)
find_package (Threads)

#override cmake internal compiler settings
set(CMAKE_C_COMPILER "/usr/bin/clang" CACHE string "clang compiler" FORCE)

#add the include folder, so that the compiler will find all files there
include_directories(include)

#take all .c files from src folder and add them to the SOURCE_FILE variable
file(GLOB SOURCE_FILES src/*.c)
#take all .h files from include folder and add them to the HEADER_FILES variable
file(GLOB HEADER_FILES include/*.h)

#define the compiler options 
add_compile_options(
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
    -fno-optimize-sibling-calls
)

#these definitions enable some new standard library functions
add_definitions(-D_XOPEN_SOURCE -D_XOPEN_SOURCE_EXTENDED -D_GNU_SOURCE)

#Enable Clang Debug Tool Support, only one of this two will work at the same time
if(sanitizeThread)
    # -fsanitize=thread enables the clang thread sanitizer
    # see http://clang.llvm.org/docs/ThreadSanitizer.html for more details
    add_compile_options(
        -fsanitize=thread
        -fPIE
    )
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread")
endif()
if(sanitizeAddress)
    # -fsanitize=address enables the clang address sanatizer
    # see http://clang.llvm.org/docs/AddressSanitizer.html for more details
    add_compile_options(
        -fsanitize=address
    )
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
endif()

#define the output executable and the files from which it is created
add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

#add dynamic linked libraries
target_link_libraries (${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})

#chmod +x binaries -- just to be safe
add_custom_target(makeExecutable ALL
    COMMAND chmod +x ${CMAKE_CURRENT_SOURCE_DIR}/ressources/myGroup-*
)

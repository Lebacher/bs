target_sources(puthreads
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/queue.c
  ${CMAKE_CURRENT_SOURCE_DIR}/tcb.c
  ${CMAKE_CURRENT_SOURCE_DIR}/threads.c)

target_include_directories(puthreads
  PUBLIC
  ${CMAKE_SOURCE_DIR}/lib/puthreads)

target_compile_options(puthreads
  PRIVATE
  -std=c11
  #-Wall
  -g
  -O0
  -Wconversion
  -Wsign-conversion
  -Wno-newline-eof
  -Wno-array-bounds
  )

target_compile_definitions(puthreads
  PRIVATE
  _XOPEN_SOURCE=700
  _DEFAULT_SOURCE
)

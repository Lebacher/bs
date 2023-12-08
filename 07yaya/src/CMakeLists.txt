target_sources(scheduler
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/main.c
  ${CMAKE_CURRENT_SOURCE_DIR}/demo.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sem.c
  )

target_include_directories(scheduler
  PRIVATE
  "${CMAKE_SOURCE_DIR}/include")

target_compile_options(scheduler
  PRIVATE
  -std=c11
  -O0
  -g
  -pedantic
  -Wall
  -Wextra
  -Wconversion
  -Wsign-conversion
  -fcolor-diagnostics
  -fsanitize=address
  -fno-omit-frame-pointer
  -fno-optimize-sibling-calls
  )

target_compile_definitions(scheduler
  PRIVATE
  _XOPEN_SOURCE
  _XOPEN_SOURCE_EXTENDED
  _DEFAULT_SOURCE
  )

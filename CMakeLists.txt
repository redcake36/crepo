cmake_minimum_required(VERSION 3.16)
project(statdump C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(statdump_lib
  src/io.c
  src/join.c
  src/sort.c
  src/print.c
)
target_include_directories(statdump_lib PUBLIC include)

add_executable(statdump_tool src/main.c)
target_link_libraries(statdump_tool PRIVATE statdump_lib)

add_executable(test_runner tests/test_runner_upd.c)
target_link_libraries(test_runner PRIVATE statdump_lib)

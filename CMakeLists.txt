cmake_minimum_required(VERSION 3.9)
project(clangtags)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_PREFIX_PATH "/usr/local/Cellar/llvm/6.0.1")

find_package(Clang)
find_package(LLVM REQUIRED CONFIG)

include_directories(${CLANG_INCLUDE_DIRS})

add_executable(clangtags main.cpp json.hpp)
target_link_libraries(clangtags libclang)
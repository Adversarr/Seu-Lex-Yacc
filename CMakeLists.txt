cmake_minimum_required(VERSION 3.21)
project(SeuLexYacc)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(sly)

add_subdirectory(test)

add_subdirectory(standalone)


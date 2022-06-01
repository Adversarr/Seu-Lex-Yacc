cmake_minimum_required(VERSION 3.0)
project(SeuLexYacc)

SET(CMAKE_BUILD_TYPE "Debug")

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(sly)

add_subdirectory(test)

add_subdirectory(standalone)



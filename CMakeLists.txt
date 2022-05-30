cmake_minimum_required(VERSION 3.0)
project(SeuLexYacc)
set(CMAKE_CXX_STANDARD 20)

SET(CMAKE_BUILD_TYPE "Debug")

add_subdirectory(sly)

add_subdirectory(test)

add_subdirectory(standalone)



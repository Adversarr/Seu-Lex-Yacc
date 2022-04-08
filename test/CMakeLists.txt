link_libraries(sly)
include_directories(${CMAKE_HOME_DIRECTORY}/sly/include)

add_executable(test1 test1.cpp)
add_executable(regex_lr regex_lr.cpp)
add_executable(test2 test2.cpp)
include_directories(${CMAKE_HOME_DIRECTORY}/sly/include)

link_libraries(sly)

add_executable(SeuLex lex.cpp)

add_executable(SeuYacc yacc.cpp)

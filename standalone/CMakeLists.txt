include_directories(${CMAKE_HOME_DIRECTORY}/sly/include)
link_libraries(sly)
add_executable(SeuLexYacc main_program.cpp)

target_compile_definitions(SeuLexYacc PUBLIC -DCPP_INPUT_ORIGINAL_PATH=\"${CPP_INPUT_ORIGINAL_PATH}\")
target_compile_definitions(SeuLexYacc PUBLIC -DTINY_LEX_INPUT_ORIGINAL_PATH=\"${TINY_LEX_INPUT_ORIGINAL_PATH}\")
target_compile_definitions(SeuLexYacc PUBLIC -DTINY_YACC_INPUT_ORIGINAL_PATH=\"${TINY_YACC_INPUT_ORIGINAL_PATH}\")
target_compile_definitions(SeuLexYacc PUBLIC -DC9_LEX_INPUT_ORIGINAL_PATH=\"${C9_LEX_INPUT_ORIGINAL_PATH}\")
target_compile_definitions(SeuLexYacc PUBLIC -DC9_YACC_INPUT_ORIGINAL_PATH=\"${C9_YACC_INPUT_ORIGINAL_PATH}\")
target_compile_definitions(SeuLexYacc PUBLIC -DC99_LEX_INPUT_ORIGINAL_PATH=\"${C99_LEX_INPUT_ORIGINAL_PATH}\")
target_compile_definitions(SeuLexYacc PUBLIC -DC99_YACC_INPUT_ORIGINAL_PATH=\"${C99_YACC_INPUT_ORIGINAL_PATH}\")
target_compile_definitions(SeuLexYacc PUBLIC -DOUTPUT_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}\")
file(TOUCH out.cpp)
file(TOUCH out_precompile.cpp)

add_executable(output out.cpp)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
cmake_minimum_required(VERSION 3.25)
set(CMAKE_BUILD_TYPE Debug)

project(c-compiler)

add_executable(c-compiler src/main.c src/lexer.c include/lexer.h src/parser.c include/parser.h src/assembly.c include/assembly.h src/code_emit.c include/code_emit.h src/intermediate_rep.c include/intermediate_rep.h src/hash_table.c include/hash_table.h src/sa_variable_resolution.c include/sa_variable_resolution.h src/sa_loop_labeling.c include/sa_loop_labeling.h src/arena.c include/arena.h src/stack.c include/stack.h src/sa_type_check.c include/sa_type_check.h)

add_custom_command(TARGET c-compiler POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:c-compiler>
        ${CMAKE_BINARY_DIR}/../test/build-osx)

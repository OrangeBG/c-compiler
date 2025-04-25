cmake_minimum_required(VERSION 3.28)
set(CMAKE_BUILD_TYPE Debug)

project(c-compiler)

add_executable(c-compiler src/main.c src/lexer.c include/lexer.h src/parser.c include/parser.h src/assembly.c include/assembly.h src/code_emit.c include/code_emit.h src/intermediate_rep.c include/intermediate_rep.h)
add_custom_command(TARGET c-compiler POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:c-compiler>
        ${CMAKE_BINARY_DIR}/../test/build-osx)

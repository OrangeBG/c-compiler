cmake_minimum_required(VERSION 3.28)
set(CMAKE_BUILD_TYPE Debug)

project(c-compiler)

add_executable(c-compiler src/main.c src/lexer.c include/lexer.h src/parser.c include/parser.h src/assembly.c include/assembly.h)

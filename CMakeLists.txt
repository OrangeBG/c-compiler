set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
cmake_minimum_required(VERSION 3.25)
set(CMAKE_BUILD_TYPE Debug)
project(c-compiler)

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB SRC_FILES "${PROJECT_SOURCE_DIR}/src/*.c")
file(GLOB INCLUDE_FILES "${PROJECT_SOURCE_DIR}/include/*.h")

add_executable(c-compiler ${INCLUDE_FILES} ${SRC_FILES})

add_custom_command(TARGET c-compiler POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:c-compiler>
        ${CMAKE_BINARY_DIR}/../test/build)

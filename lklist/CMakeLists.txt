cmake_minimum_required(VERSION 3.10)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(SRC_FILES "")

list(
    APPEND SRC_FILES
    include/lklist.h
        src/lklist.c
)

add_library(lklist ${SRC_FILES})

target_include_directories(lklist PUBLIC include)

if(MSVC)
    target_compile_options(lklist PRIVATE /W4)
else()
    # GCC and Clang
    target_compile_options(lklist PRIVATE -Wall -Wpointer-arith)
endif()

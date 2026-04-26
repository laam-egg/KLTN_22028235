cmake_minimum_required(VERSION 3.10)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(SRC_FILES "")

list(
    APPEND SRC_FILES
    include/murmur3.h
        src/murmur3.c
)

add_library(murmur3 ${SRC_FILES})

set_property(
    TARGET murmur3
    PROPERTY    C_STANDARD              99
    PROPERTY    C_STANDARD_REQUIRED     ON
)

target_include_directories(murmur3 PUBLIC include)

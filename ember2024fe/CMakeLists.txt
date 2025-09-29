cmake_minimum_required(VERSION 3.10)
project(EMBER2024_FEATURE_EXTRACTION LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(SRC_FILES "")

list(
    APPEND SRC_FILES
    include/ember2024fe.h
        src/ember2024fe.c
    include/ember2024fe/general_file_info.h
                    src/general_file_info.c
)

add_library(ember2024fe ${SRC_FILES})

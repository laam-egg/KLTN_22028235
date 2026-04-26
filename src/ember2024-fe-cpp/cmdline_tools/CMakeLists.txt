cmake_minimum_required(VERSION 3.26)
set(TARGET_NAME cmdline_tools)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_FILES "")

list(
    APPEND SRC_FILES
    include/cmdline_tools/argv.h
                    src/argv.cpp
)

add_library(${TARGET_NAME} ${SRC_FILES})

set_property(
    TARGET ${TARGET_NAME}
    PROPERTY    CXX_STANDARD            17
    PROPERTY    CXX_STANDARD_REQUIRED   ON
)

target_include_directories(${TARGET_NAME} PUBLIC include)

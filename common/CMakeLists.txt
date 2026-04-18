cmake_minimum_required(VERSION 3.24)

set(TARGET_NAME pmd_service_common)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${TARGET_NAME} STATIC
    include/utils.h
        src/utils.cpp
)

target_include_directories(${TARGET_NAME} PUBLIC include)

target_link_libraries(
    ${TARGET_NAME}
    user32
    kernel32
)

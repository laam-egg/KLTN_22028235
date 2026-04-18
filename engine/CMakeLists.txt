cmake_minimum_required(VERSION 3.24)

set(TARGET_NAME pmd_engine_communication)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${TARGET_NAME} STATIC
    include/pmd_engine/engine.h
        src/engine.cpp

        src/resources.h
        src/resources.cpp
)

target_include_directories(${TARGET_NAME} PUBLIC include src)

target_link_libraries(${TARGET_NAME} PUBLIC
    user32
    kernel32
)

file(GLOB_RECURSE RESOURCE_FILES "${CMAKE_CURRENT_LIST_DIR}/resources/*")

cmrc_add_resource_library(
    engine_resources
    NAMESPACE engine_resources
    WHENCE ${CMAKE_CURRENT_LIST_DIR}/resources
    ${RESOURCE_FILES}
)

target_link_libraries(${TARGET_NAME} PRIVATE engine_resources)

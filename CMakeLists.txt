cmake_minimum_required(VERSION 3.24)
project(pmd_service)

# https://stackoverflow.com/a/40947954/13680015
string(LENGTH "${CMAKE_SOURCE_DIR}/" SOURCE_PATH_SIZE)
add_definitions("-DSOURCE_PATH_SIZE=${SOURCE_PATH_SIZE}")

add_subdirectory(main)

cmake_minimum_required(VERSION 3.10)
project("ember2024cpp")

# https://stackoverflow.com/a/40947954/13680015
string(LENGTH "${CMAKE_SOURCE_DIR}/" SOURCE_PATH_SIZE)
add_definitions("-DSOURCE_PATH_SIZE=${SOURCE_PATH_SIZE}")

add_subdirectory("lklist")
add_subdirectory("murmur3")
add_subdirectory("featurehasher")
add_subdirectory("caching")
add_subdirectory("common")
add_subdirectory("pefile")
add_subdirectory("core")
add_subdirectory("main")

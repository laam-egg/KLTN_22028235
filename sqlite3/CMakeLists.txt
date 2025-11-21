cmake_minimum_required(VERSION 3.24)

add_library(sqlite3 STATIC sqlite3.c sqlite3.h)

# Export source dir as an interface property
target_include_directories(sqlite3 INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_compile_definitions(sqlite3 PRIVATE SQLITE_THREADSAFE=1)

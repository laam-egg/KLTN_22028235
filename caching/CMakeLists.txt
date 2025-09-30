cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_FILES "")

list(
    APPEND SRC_FILES
    include/caching.h
        src/caching.cpp

    include/caching/tuple_hash.h
    include/caching/is_cache.h
    include/caching/lru_cache.h
)

add_library(caching ${SRC_FILES})

target_include_directories(caching PUBLIC include)

if(MSVC)
    target_compile_options(caching PRIVATE /W4)
else()
    # GCC and Clang
    target_compile_options(caching PRIVATE -Wall -Wpointer-arith)
endif()

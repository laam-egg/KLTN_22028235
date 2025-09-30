cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_FILES "")

list(
    APPEND SRC_FILES
    include/efe/pefile.h
            src/pefile.cpp
    include/efe/pefile/common.h
                    src/common.cpp
)

add_library(efe_pefile ${SRC_FILES})

target_include_directories(efe_pefile PUBLIC include)

target_link_libraries(efe_pefile PUBLIC caching)

cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_FILES "")

list(
    APPEND SRC_FILES
    include/efe/common.h
            src/common.cpp
    include/efe/common/blockstream.h
                    src/blockstream.cpp
    include/efe/common/memoryblockstream.h
                    src/memoryblockstream.cpp
)

add_library(efe_common ${SRC_FILES})

target_include_directories(efe_common PUBLIC include)

target_link_libraries(efe_common PUBLIC lklist)

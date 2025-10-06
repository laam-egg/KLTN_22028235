cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(SRC_FILES "")
list(
    APPEND SRC_FILES
    include/efe/featurehasher.h
            src/featurehasher.cpp
)

add_library(efe_featurehasher ${SRC_FILES})

set_property(
    TARGET efe_featurehasher
    PROPERTY    CXX_STANDARD            17
    PROPERTY    CXX_STANDARD_REQUIRED   ON
)

target_include_directories(efe_featurehasher PUBLIC include)

target_link_libraries(efe_featurehasher PUBLIC murmur3 efe_common)









set(CLI_SRC_FILES "")
list(
    APPEND CLI_SRC_FILES
    src/fh.cpp
)
add_executable(fh ${CLI_SRC_FILES})

set_property(
    TARGET fh
    PROPERTY    CXX_STANDARD            17
    PROPERTY    CXX_STANDARD_REQUIRED   ON
)

target_link_libraries(fh PUBLIC efe_featurehasher)

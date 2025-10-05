cmake_minimum_required(VERSION 3.26)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_FILES "")

list(
    APPEND SRC_FILES
    include/efe/common.h
            src/common.cpp
    include/efe/common/nop.h
    include/efe/common/logging.h
            src/logging.cpp
    include/efe/common/meth.h
            src/meth.cpp
    include/efe/common/blockstream.h
                    src/blockstream.cpp
    include/efe/common/memoryblockstream.h
                    src/memoryblockstream.cpp
    include/efe/common/bytecounter.h
                    src/bytecounter.cpp
    include/efe/common/bytecatcher.h
                    src/bytecatcher.cpp
    include/efe/common/slidingbytewindow.h
                    src/slidingbytewindow.cpp
    include/efe/common/shannonentropycalculator.h
                    src/shannonentropycalculator.cpp
)

add_library(efe_common ${SRC_FILES})

set_property(
    TARGET efe_common
    PROPERTY    CXX_STANDARD            17
    PROPERTY    CXX_STANDARD_REQUIRED   ON
)

target_include_directories(efe_common PUBLIC include)

target_link_libraries(efe_common PUBLIC lklist)

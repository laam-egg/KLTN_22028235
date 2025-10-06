cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_FILES "")

list(
    APPEND SRC_FILES
    include/efe/core.h
            src/core.cpp
    include/efe/core/common.h
                src/common.cpp
    include/efe/core/f_base.h
                src/f_base.cpp
    include/efe/core/f_general_file_info.h
                src/f_general_file_info.cpp
    include/efe/core/f_byte_entropy_histogram.h
                src/f_byte_entropy_histogram.cpp
    include/efe/core/f_string_extractor.h
                src/f_string_extractor.cpp
    include/efe/core/f_section_info.h
                src/f_section_info.cpp
    include/efe/core/f_imports_info.h
                src/f_imports_info.cpp
)

add_library(efe_core ${SRC_FILES})

target_include_directories(efe_core PUBLIC include)

target_link_libraries(efe_core PUBLIC efe_common efe_pefile efe_featurehasher)

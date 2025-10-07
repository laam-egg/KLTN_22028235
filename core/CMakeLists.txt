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
    include/efe/core/f_exports_info.h
                src/f_exports_info.cpp

    include/efe/core/f_header_file_info/machine_type_encoding.h
                src/f_header_file_info/machine_type_encoding.cpp
    include/efe/core/f_header_file_info/subsystem_type_encoding.h
                src/f_header_file_info/subsystem_type_encoding.cpp
    include/efe/core/f_header_file_info/dll_characteristics_encoding.h
                src/f_header_file_info/dll_characteristics_encoding.cpp
    include/efe/core/f_header_file_info/coff_characteristics_encoding.h
                src/f_header_file_info/coff_characteristics_encoding.cpp
    include/efe/core/f_header_file_info/dos_header_encoding.h
                src/f_header_file_info/dos_header_encoding.cpp
    include/efe/core/f_header_file_info.h
                src/f_header_file_info.cpp
    include/efe/core/f_data_directories.h
                src/f_data_directories.cpp
    include/efe/core/f_rich_header.h
                src/f_rich_header.cpp
    include/efe/core/f_authenticode_signature.h
                src/f_authenticode_signature.cpp
    include/efe/core/f_pe_format_warnings.h
                src/f_pe_format_warnings.cpp

    include/efe/core/all_feature_types.h
                src/all_feature_types.cpp
)

add_library(efe_core ${SRC_FILES})

target_include_directories(efe_core PUBLIC include)

target_link_libraries(efe_core PUBLIC efe_common efe_pefile efe_featurehasher)

target_link_libraries(efe_core PUBLIC re2::re2)

cmake_minimum_required(VERSION 3.16)
set(TARGET_NAME efe_shared)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(GenerateExportHeader)

if (INSTALL_SHARED)
    include(GNUInstallDirs)
endif()

######################
#### SOURCE FILES ####
######################

set(SRC_FILES "")
list(
    APPEND SRC_FILES
    
    include/efe/shared.h
            src/shared.cpp
)

add_library(${TARGET_NAME} SHARED ${SRC_FILES})

target_include_directories(
    ${TARGET_NAME}
    PUBLIC
    include
    ${CMAKE_CURRENT_BINARY_DIR} # for the generated header: ${TARGET_NAME}_export.h
)

##########################
#### LINKED LIBRARIES ####
##########################

set(LINKED_LIBRARIES "")

if (NOT WIN32)
    list(APPEND LINKED_LIBRARIES
        pthread      # required on Linux
        m            # math library
        rt           # real-time library
        dl           # dynamic loader
    )
endif()

list(APPEND LINKED_LIBRARIES
    efe_core
)

target_link_libraries(${TARGET_NAME} PRIVATE ${LINKED_LIBRARIES})

######################
#### SHARE CONFIG ####
######################

set_target_properties(${TARGET_NAME} PROPERTIES
    PUBLIC_HEADER include/efe/shared.h
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN 1
)

generate_export_header(${TARGET_NAME})
target_include_directories(
    ${TARGET_NAME}
    PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
) # for the generated header: ${TARGET_NAME}_export.h

if (INSTALL_SHARED)
    # Other packages/executables could find this library:
    # find_package(efe_shared REQUIRED)
    # target_link_libraries(their_target PRIVATE efe::efe_shared)

    install(TARGETS ${TARGET_NAME}
        EXPORT ${TARGET_NAME}Targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/efe
    )

    install(EXPORT ${TARGET_NAME}Targets
        FILE ${TARGET_NAME}Config.cmake
        NAMESPACE efe::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${TARGET_NAME}
    )
endif()

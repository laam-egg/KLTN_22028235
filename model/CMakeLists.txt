set(LGBM_ROOT "$ENV{LGBM_ROOT}")
if (NOT LGBM_ROOT)
    message(ERROR "Environment variable LGBM_ROOT not defined")
else()
    message(STATUS "LightGBM root: ${LGBM_ROOT}")
endif()

set(SRC_FILES "")

list(
    APPEND SRC_FILES
    include/ember2024_lgbm/model.h
                        src/model.cpp
                        src/direct_model.h
                        src/direct_model.cpp
                        src/resources.h
                        src/resources.cpp
)

add_library(ember2024_lgbm_model ${SRC_FILES})

target_include_directories(ember2024_lgbm_model PUBLIC
    include
    "${LGBM_ROOT}/include"
)
target_include_directories(ember2024_lgbm_model PRIVATE src)

if (MSVC)
    # MSVC with Ninja
    set(LIGHTGBM_STATIC_LIB_FILE
        "${LGBM_ROOT}/lib_lightgbm.lib")
else()
    # Linux / macOS
    set(LIGHTGBM_STATIC_LIB_FILE
        "${LGBM_ROOT}/lib_lightgbm.a")
endif()

if (WIN32)
    # Fix errors linking against LightGBM (probably) on Windows
    target_link_libraries(ember2024_lgbm_model PUBLIC ws2_32 iphlpapi)
endif()

set(LINKED_LIBRARIES "${LIGHTGBM_STATIC_LIB_FILE}" efe_core)

if (NOT WIN32)
    list(APPEND LINKED_LIBRARIES
        pthread      # required on Linux
        m            # math library
        rt           # real-time library
        dl           # dynamic loader
    )
endif()

find_package(OpenMP REQUIRED)

if(OpenMP_CXX_FOUND)
    list(APPEND LINKED_LIBRARIES OpenMP::OpenMP_CXX)
endif()

target_link_libraries(ember2024_lgbm_model PUBLIC ${LINKED_LIBRARIES})

# STATICALLY EMBEDDED RESOURCES

file(GLOB_RECURSE RESOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/*")

cmrc_add_resource_library(
    model_resources
    NAMESPACE model_resources
    WHENCE ${CMAKE_CURRENT_SOURCE_DIR}/resources
    ${RESOURCE_FILES}
)

target_link_libraries(ember2024_lgbm_model PRIVATE model_resources)

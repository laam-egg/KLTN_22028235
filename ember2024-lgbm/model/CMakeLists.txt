set(LGBM_ROOT "$ENV{LGBM_ROOT}")
if (NOT LGBM_ROOT)
    message(ERROR "Environment variable LGBM_ROOT not defined")
else()
    message(STATUS "LightGBM root: ${LGBM_ROOT}")
endif()


if (MSVC)
    set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)
    if (BUILD_TESTING)
        # Stupid GoogleTest won't let us link static CRT easily
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    else()
        # Link statically to ensure portability
        add_compile_options("/MT$<$<CONFIG:Release>:>")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /fp:precise /DNDEBUG") # /GL
    set(CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE}   /O2 /fp:precise /DNDEBUG") # /GL
    # set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG")
    # set(CMAKE_EXE_LINKER_FLAGS_RELEASE    "${CMAKE_EXE_LINKER_FLAGS_RELEASE}    /LTCG")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -fno-unsafe-math-optimizations -fno-fast-math -DNDEBUG") # -flto
    set(CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE}   -O3 -fno-unsafe-math-optimizations -fno-fast-math -DNDEBUG") # -flto
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
    list(APPEND LINKED_LIBRARIES OpenMP::OpenMP_C OpenMP::OpenMP_CXX)
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











if (MSVC)
    if (NOT BUILD_TESTING)
        # Also set these for older CMake or stubborn dependencies
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
        
        # Remove any /MD flags that might be set
        string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
        string(REPLACE "/MDd" "/MTd" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
        string(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
        string(REPLACE "/MDd" "/MTd" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")

        set(CompilerFlags
            CMAKE_CXX_FLAGS
            CMAKE_CXX_FLAGS_DEBUG
            CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL
            CMAKE_CXX_FLAGS_RELWITHDEBINFO
            CMAKE_C_FLAGS
            CMAKE_C_FLAGS_DEBUG
            CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_MINSIZEREL
            CMAKE_C_FLAGS_RELWITHDEBINFO
        )
        foreach(CompilerFlag ${CompilerFlags})
            string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
            set(${CompilerFlag} "${${CompilerFlag}}" CACHE STRING "msvc compiler flags" FORCE)
            message("MSVC flags: ${CompilerFlag}:${${CompilerFlag}}")
        endforeach()
    endif()
endif()

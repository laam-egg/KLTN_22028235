cmake_minimum_required(VERSION 3.24)
project(ember2024_lgbm)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTING "Enable tests" OFF)

if (MSVC)
    add_compile_options("/MT$<$<CONFIG:Release>:>")
    set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)
    if (BUILD_TESTING)
        # Stupid GoogleTest won't let us link static CRT easily
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    else()
        # Link statically to ensure portability
        add_compile_options("/MT$<$<CONFIG:Release>:>")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /EHsc /fp:precise /DNDEBUG") # /GL
    set(CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE}   /O2 /fp:precise /DNDEBUG") # /GL
    # set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG")
    # set(CMAKE_EXE_LINKER_FLAGS_RELEASE    "${CMAKE_EXE_LINKER_FLAGS_RELEASE}    /LTCG")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -fexceptions -funwind-tables -fno-unsafe-math-optimizations -fno-fast-math -DNDEBUG") # -flto
    set(CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE}   -O3 -fno-unsafe-math-optimizations -fno-fast-math -DNDEBUG") # -flto
endif()

add_subdirectory("cmakerc")
add_subdirectory("model")
add_subdirectory("feature_engineering")
add_subdirectory("demo")

# Only available on Win32
if (WIN32)
    add_subdirectory("pmd_engine")
    add_subdirectory("pmd_engine_v2")
endif()

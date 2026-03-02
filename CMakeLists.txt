cmake_minimum_required(VERSION 3.10)
project("ember2024-fe-cpp")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BUILD_TESTING "Enable tests" OFF)

if (MSVC)
    if (BUILD_TESTING)
        # Stupid GoogleTest won't let us link static CRT easily
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    else()
        # Link statically to ensure portability
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

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# https://stackoverflow.com/a/40947954/13680015
string(LENGTH "${CMAKE_SOURCE_DIR}/" SOURCE_PATH_SIZE)
add_definitions("-DSOURCE_PATH_SIZE=${SOURCE_PATH_SIZE}")

if (BUILD_TESTING)
    message(STATUS "BUILDING TESTS")
    include(CTest)
    enable_testing()

    add_subdirectory(sqlite3)
    add_subdirectory(googletest)
    add_subdirectory(tests)
endif()



# add_subdirectory("lklist")
add_subdirectory("murmur3")
add_subdirectory("featurehasher")
# add_subdirectory("caching")
add_subdirectory("thirdparty")
add_subdirectory("common")
add_subdirectory("pefile")
add_subdirectory("core")
add_subdirectory("demo")
add_subdirectory("shared")
add_subdirectory("shared_demo")

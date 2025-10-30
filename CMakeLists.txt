cmake_minimum_required(VERSION 3.10)
project("ember2024-fe-cpp")

if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# https://stackoverflow.com/a/40947954/13680015
string(LENGTH "${CMAKE_SOURCE_DIR}/" SOURCE_PATH_SIZE)
add_definitions("-DSOURCE_PATH_SIZE=${SOURCE_PATH_SIZE}")

option(BUILD_TESTING "Enable tests" OFF)
if (BUILD_TESTING)
    message(STATUS "BUILDING TESTS")
    include(CTest)
    include(FetchContent)
    enable_testing()

    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
    )
    FetchContent_MakeAvailable(googletest)

    add_subdirectory(tests)
endif()



add_subdirectory("lklist")
add_subdirectory("murmur3")
add_subdirectory("featurehasher")
add_subdirectory("caching")
add_subdirectory("thirdparty")
add_subdirectory("common")
add_subdirectory("pefile")
add_subdirectory("core")
add_subdirectory("demo")

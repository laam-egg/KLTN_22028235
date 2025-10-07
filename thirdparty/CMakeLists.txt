cmake_minimum_required(VERSION 3.24)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Abseil, which RE2 depends on

set(ABSL_URL        "https://github.com/abseil/abseil-cpp.git")
set(ABSL_VERSION    "2af4687a56889e8a1db68fdf095d9a9da2c475c3")

FetchContent_Declare(absl
    GIT_REPOSITORY  "${ABSL_URL}"
    GIT_TAG         "${ABSL_VERSION}"
)

set(ABSL_BUILD_TESTING              OFF CACHE BOOL "" FORCE)
set(ABSL_USE_GOOGLETEST_HEAD        OFF CACHE BOOL "" FORCE)
set(ABSL_ENABLE_INSTALL             ON  CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(absl)

# RE2

set(RE2_URL         "https://github.com/google/re2.git")
set(RE2_VERSION     "61c4644171ee6b480540bf9e569cba06d9090b4b")

FetchContent_Declare(re2
    GIT_REPOSITORY  "${RE2_URL}"
    GIT_TAG         "${RE2_VERSION}"
)

set(RE2_TEST                        OFF CACHE BOOL "Enable tests" FORCE)
set(RE2_BENCHMARK                   OFF CACHE BOOL "" FORCE)
set(RE2_BUILD_TESTING               OFF CACHE BOOL "" FORCE)
set(RE2_INSTALL                     ON  CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(re2)

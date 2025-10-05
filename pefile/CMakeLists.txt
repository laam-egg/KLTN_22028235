cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_FILES "")

list(
    APPEND SRC_FILES
    include/efe/pefile.h
            src/pefile.cpp
    include/efe/pefile/common.h
                    src/common.cpp
)

add_library(efe_pefile ${SRC_FILES})

target_include_directories(efe_pefile PUBLIC include)

target_link_libraries(efe_pefile PUBLIC efe_common caching)

include(FetchContent)






# LIEF
# https://lief.re//doc/latest/installation.html

set(LIEF_GIT_URL "https://github.com/lief-project/LIEF.git")
set(LIEF_VERSION 0.17.0)
FetchContent_Declare(LIEF
    GIT_REPOSITORY  "${LIEF_GIT_URL}"
    GIT_TAG         "${LIEF_VERSION}"
)

# https://lief.re//doc/latest/compilation.html#cmake-options
set(LIEF_DOC                        OFF        CACHE BOOL "" FORCE)
set(LIEF_TESTS                      OFF CACHE BOOL "Enable tests" FORCE)
set(LIEF_PYTHON_API                 OFF CACHE BOOL "Enable Python Bindings" FORCE)
set(LIEF_C_API                      ON  CACHE BOOL "C API" FORCE)
set(LIEF_EXAMPLES                   OFF CACHE BOOL "Build LIEF C++ examples" FORCE)
set(LIEF_FORCE32                    OFF CACHE BOOL "Force build LIEF 32 bits version" FORCE)
set(LIEF_USE_CCACHE                 ON  CACHE BOOL "Use ccache to speed up compilation" FORCE)
set(LIEF_EXTRA_WARNINGS             OFF CACHE BOOL "Enable extra warning from the compiler" FORCE)
set(LIEF_LOGGING                    OFF CACHE BOOL "Enable logging" FORCE)
set(LIEF_LOGGING_DEBUG              OFF CACHE BOOL "Enable debug logging" FORCE)
set(LIEF_ENABLE_JSON                ON  CACHE BOOL "Enable JSON-related APIs" FORCE)
set(LIEF_OPT_NLOHMANN_JSON_EXTERNAL OFF CACHE BOOL "Use nlohmann/json externally" FORCE)
set(LIEF_FORCE_API_EXPORTS          OFF CACHE BOOL "Force exports of API symbols" FORCE)
set(LIEF_PY_LIEF_EXT                OFF CACHE BOOL "Use a pre-installed version of LIEF for the bindings" FORCE)
set(LIEF_RUST_API                   OFF CACHE BOOL "Generate the C++ bridge for Rust's cxx" FORCE)
set(LIEF_DISABLE_EXCEPTIONS         ON  CACHE BOOL "Disable C++ exceptions on the core library" FORCE)
set(LIEF_SO_VERSION                 OFF CACHE BOOL "Embed versioning for LIEF shared library target" FORCE)
set(LIEF_DISABLE_FROZEN             OFF CACHE BOOL "Disable Frozen even if it is supported" FORCE)
set(LIEF_ELF                        OFF CACHE BOOL "Build LIEF with ELF module" FORCE)
set(LIEF_PE                         ON  CACHE BOOL "Build LIEF with PE module" FORCE)
set(LIEF_COFF                       ON  CACHE BOOL "Build LIEF with COFF module" FORCE)


if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded" CACHE STRING "CRT option")
endif()


FetchContent_MakeAvailable(LIEF)

target_link_libraries(efe_pefile PUBLIC LIEF::LIEF)

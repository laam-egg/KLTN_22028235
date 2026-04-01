set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

set(BUILD_TESTING ${BUILD_TESTING} CACHE BOOL "Forwarded test option" FORCE)

FetchContent_Declare(
    ember2024_fe_cpp
    GIT_REPOSITORY https://github.com/pe-malware-detection/ember2024-fe-cpp.git
    GIT_TAG 423ff0d111f7a77867922298648370a0a1c09b63
)

FetchContent_MakeAvailable(ember2024_fe_cpp)

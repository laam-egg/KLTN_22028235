include(FetchContent)

FetchContent_Declare(
    CMakeRC
    GIT_REPOSITORY https://github.com/vector-of-bool/cmrc.git
    GIT_TAG 952ffddba731fc110bd50409e8d2b8a06abbd237
)
FetchContent_MakeAvailable(CMakeRC)

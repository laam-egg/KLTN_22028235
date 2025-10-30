cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_FILES "")

list(APPEND SRC_FILES
    "src/main.cpp"
    "src/e2e.test.cpp"
)

add_executable(efe_e2e_test ${SRC_FILES})

set(LINKED_LIBRARIES "")

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

list(APPEND LINKED_LIBRARIES
    efe_core
    GTest::gtest_main
)

target_link_libraries(efe_e2e_test PRIVATE ${LINKED_LIBRARIES})

include(GoogleTest)
gtest_discover_tests(efe_e2e_test)

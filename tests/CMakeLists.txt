cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_FILES "")

list(APPEND SRC_FILES
    "src/main.cpp"
    "src/e2e.test.cpp"

    "include/pe_sample_reader/pe_sample.h"
        "src/pe_sample_reader/pe_sample.cpp"

    "include/pe_sample_reader/pe_sample_reader.h"
        "src/pe_sample_reader/pe_sample_reader.cpp"

    "include/testing/core/fixtures/efe_test_data_fixture.h"
                "src/core/fixtures/efe_test_data_fixture.cpp"
    
    "src/core/all_features.test.cpp"

    "src/featurehasher/featurehasher.test.cpp"

    "src/common/bytecounter.test.cpp"
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

set(TEST_DATA_DIR ${CMAKE_CURRENT_BINARY_DIR}/data)
file(
    COPY ${CMAKE_CURRENT_LIST_DIR}/resources
    DESTINATION ${TEST_DATA_DIR}
)
target_compile_definitions(efe_e2e_test
    PRIVATE TEST_DATA_DIR="${TEST_DATA_DIR}"
)

target_include_directories(efe_e2e_test PRIVATE include src sqlite3)
target_link_libraries(efe_e2e_test PRIVATE sqlite3)

include(GoogleTest)
# gtest_discover_tests(efe_e2e_test PROPERTIES DISCOVERY_TIMEOUT 15)
add_test(NAME all_e2e_tests COMMAND efe_e2e_test)

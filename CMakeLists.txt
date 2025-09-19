cmake_minimum_required(VERSION 3.10)
project(LightGBM_Cpp_Example)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIGHTGBM_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/../LightGBM/include")
set(LIGHTGBM_STATIC_LIB_FILE "${CMAKE_CURRENT_LIST_DIR}/../LightGBM/lib_lightgbm.a")

include_directories(${LIGHTGBM_INCLUDE_DIR})

set(SRC_FILES main.cpp)

add_executable(my_program ${SRC_FILES})

set(LINKED_LIBRARIES
    "${LIGHTGBM_STATIC_LIB_FILE}"
    pthread      # required on Linux
    m            # math library
    rt           # real-time library
    dl           # dynamic loader
)

find_package(OpenMP REQUIRED)

if(OpenMP_CXX_FOUND)
    list(APPEND LINKED_LIBRARIES OpenMP::OpenMP_CXX)
endif()

target_link_libraries(my_program PUBLIC ${LINKED_LIBRARIES})

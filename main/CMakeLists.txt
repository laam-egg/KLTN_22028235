cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIGHTGBM_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../LightGBM/include")

if (WIN32)
    # Windows: MSVC builds put static lib in Release/ by default
    set(LIGHTGBM_STATIC_LIB_FILE
        "${CMAKE_CURRENT_LIST_DIR}/../../LightGBM/Release/lib_lightgbm.lib")
else()
    # Linux / macOS
    set(LIGHTGBM_STATIC_LIB_FILE
        "${CMAKE_CURRENT_LIST_DIR}/../../LightGBM/lib_lightgbm.a")
endif()

include_directories(${LIGHTGBM_INCLUDE_DIR})

set(SRC_FILES main.cpp)

add_executable(my_program ${SRC_FILES})

set(LINKED_LIBRARIES "${LIGHTGBM_STATIC_LIB_FILE}")

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

target_link_libraries(my_program PUBLIC ${LINKED_LIBRARIES})

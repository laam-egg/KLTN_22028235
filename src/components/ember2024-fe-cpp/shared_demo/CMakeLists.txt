cmake_minimum_required(VERSION 3.10)
set(TARGET_NAME "efe_shared_demo")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(SRC_FILES "")

list(APPEND SRC_FILES
    "src/main.cpp"
)

add_executable(${TARGET_NAME} ${SRC_FILES})

set(LINKED_LIBRARIES "")

if (NOT WIN32)
    list(APPEND LINKED_LIBRARIES
        pthread      # required on Linux
        m            # math library
        rt           # real-time library
        dl           # dynamic loader
    )
endif()

list(APPEND LINKED_LIBRARIES
    efe_shared
)

target_link_libraries(${TARGET_NAME} PRIVATE ${LINKED_LIBRARIES})

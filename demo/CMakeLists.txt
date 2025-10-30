cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_FILES "")

list(APPEND SRC_FILES
    "src/main.cpp"
)

add_executable(efe_demo ${SRC_FILES})

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
    efe_core
)

target_link_libraries(efe_demo PRIVATE ${LINKED_LIBRARIES})

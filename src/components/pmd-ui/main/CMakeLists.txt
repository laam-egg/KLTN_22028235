cmake_minimum_required(VERSION 3.24)

set(TARGET_NAME pmd_ui)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

##########################
##### STATIC LINKING #####
#### so that it could ####
## run on older systems ##
## without VC redist... ##
##########################

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(SOURCES
    src/main.cpp
    src/app.cpp
    src/main_view.cpp
    src/data_fetcher.cpp
    src/upload_file.cpp
)

set(INCLUDES
    include/
)

set(LIBRARIES
    bcrypt
    user32
    gdi32
    comctl32

    pmd_ui_common
    pmd_driver_communication
)




# Increase stack size
set(STACK_SIZE 16777216) # 16 MB
if (MSVC)
    add_link_options(/STACK:${STACK_SIZE})
elseif (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    if (WIN32)
        add_link_options("-Wl,--stack,${STACK_SIZE}")
    else()
        # Linux, BSD, macOS: raise stack at runtime via setrlimit()
        # add_compile_definitions(SET_STACK_AT_RUNTIME)
		# But this program only runs on Windows so...
		message(FATAL_ERROR "This program can only be built on Windows.")
    endif()
endif()

add_executable(${TARGET_NAME} WIN32 ${SOURCES})
target_include_directories(${TARGET_NAME} PRIVATE ${INCLUDES})
target_link_libraries(${TARGET_NAME} PRIVATE ${LIBRARIES})

# Enable Unicode support
target_compile_definitions(${TARGET_NAME} PRIVATE UNICODE _UNICODE)

##########################
##### STATIC LINKING #####
#### so that it could ####
## run on older systems ##
## without VC redist... ##
##########################

if(MSVC)
    target_link_options(${TARGET_NAME} PRIVATE /SUBSYSTEM:WINDOWS)
endif()
if(MINGW)
    target_link_options(${TARGET_NAME} PRIVATE -static -static-libgcc)
endif()

### JSON

include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann_json)

target_link_libraries(${TARGET_NAME} PRIVATE nlohmann_json::nlohmann_json)

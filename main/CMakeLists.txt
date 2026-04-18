cmake_minimum_required(VERSION 3.24)

set(TARGET_NAME pmd_service)

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

##########################

add_executable(${TARGET_NAME}
    include/stdafx.h
        src/stdafx.cpp

    include/CharacterEncoding.h
        src/CharacterEncoding.cpp
    
    include/ServiceBase.h
        src/ServiceBase.cpp

    include/Logging.h

    include/ServiceInstaller.h
        src/ServiceInstaller.cpp
    
    include/WindowsServiceImpl.h
        src/WindowsService.cpp
        src/WindowsServiceImpl.cpp
)

target_include_directories(${TARGET_NAME} PRIVATE include)

set(LIBRARIES
    bcrypt
)

target_link_libraries(${TARGET_NAME} PRIVATE ${LIBRARIES})

# Enable Unicode support
target_compile_definitions(${TARGET_NAME} PRIVATE UNICODE _UNICODE)

if(MSVC)
    target_link_options(${TARGET_NAME} PRIVATE /SUBSYSTEM:CONSOLE)
endif()

###############################
##### INCREASE STACK SIZE #####
###############################

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

###############################

##########################
##### STATIC LINKING #####
#### so that it could ####
## run on older systems ##
## without VC redist... ##
##########################

if(MINGW)
    target_link_options(${TARGET_NAME} PRIVATE -static -static-libgcc)
endif()

##########################

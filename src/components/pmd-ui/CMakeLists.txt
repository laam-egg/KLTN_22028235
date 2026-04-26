cmake_minimum_required(VERSION 3.24)
project(pmd_ui)

# https://stackoverflow.com/a/40947954/13680015
string(LENGTH "${CMAKE_SOURCE_DIR}/" SOURCE_PATH_SIZE)
add_definitions("-DSOURCE_PATH_SIZE=${SOURCE_PATH_SIZE}")

##########################
##### STATIC LINKING #####
#### so that it could ####
## run on older systems ##
## without VC redist... ##
##########################

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
if (MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
endif()

##########################

add_subdirectory(common)
add_subdirectory(driver)
add_subdirectory(main)

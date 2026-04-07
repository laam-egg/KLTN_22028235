cmake_minimum_required(VERSION 3.24)

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

set(TARGET_NAME e24lgbm_pmd_engine_v2)

add_library(${TARGET_NAME} SHARED
    include/pmd_engine_v2.h
        src/pmd_engine_v2.cpp
    include/pmd_engine_v2_impl.h
        src/pmd_engine_v2_impl.cpp
)

# Define export macro for DLL build
target_compile_definitions(${TARGET_NAME} PRIVATE PMD_ENGINE_V2_EXPORTS)

target_include_directories(${TARGET_NAME} PUBLIC include)

target_link_libraries(${TARGET_NAME} PRIVATE
    ember2024_lgbm_model
    efe_core
)

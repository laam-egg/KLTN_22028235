cmake_minimum_required(VERSION 3.24)

set(TARGET_NAME e24lgbm_pmd_engine)

add_library(${TARGET_NAME} SHARED
    include/e24lgbm_pmd_engine.h
        src/e24lgbm_pmd_engine.cpp
    include/e24lgbm_pmd_engine_impl.h
        src/e24lgbm_pmd_engine_impl.cpp
)

# Define export macro for DLL build
target_compile_definitions(${TARGET_NAME} PRIVATE PMD_ENGINE_EXPORTS)

target_include_directories(${TARGET_NAME} PUBLIC include)

target_link_libraries(${TARGET_NAME} PRIVATE
    ember2024_lgbm_model
    efe_core
)

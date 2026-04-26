set(SRC_FILES "")

list(
    APPEND SRC_FILES
    src/main.cpp
)

add_executable(ember2024_lgbm_demo ${SRC_FILES})

target_link_libraries(ember2024_lgbm_demo PUBLIC ember2024_lgbm_model)
target_link_libraries(ember2024_lgbm_demo PUBLIC efe_core)

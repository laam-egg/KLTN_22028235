cmake_minimum_required(VERSION 3.24)
project(ember2024_lgbm)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory("cmakerc")
add_subdirectory("model")
add_subdirectory("feature_engineering")
add_subdirectory("demo")

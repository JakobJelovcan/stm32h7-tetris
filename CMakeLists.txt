cmake_minimum_required(VERSION 3.20)

project("stm32h7-tetris" C CXX ASM)

include(cmake/st-project.cmake)

add_executable(${PROJECT_NAME})
add_st_target_properties(${PROJECT_NAME})
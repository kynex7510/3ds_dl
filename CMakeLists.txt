cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
include(CMake/CPM.cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
project(dl)

CPMAddPackage("gh:kynex7510/CTRL#2cd860a")

file(GLOB DL_SOURCES Source/*.c)
add_library(${PROJECT_NAME} STATIC ${DL_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC Include)
target_compile_options(${PROJECT_NAME} PRIVATE -O3 -Wall -Wno-switch)
target_link_libraries(${PROJECT_NAME} CTRL)

add_subdirectory(Tests)
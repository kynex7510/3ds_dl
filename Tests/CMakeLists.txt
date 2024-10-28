cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
project(dl-test)

# Doesn't seem to be supported, compile manually with:
# $DEVKITPRO/devkitARM/bin/arm-none-eabi-gcc -fPIC -shared Math.c -o Math.so
# add_library(math SHARED Math.c)

add_executable("${PROJECT_NAME}-path" Path.c)
target_link_libraries("${PROJECT_NAME}-path" PUBLIC dl)
ctr_create_3dsx("${PROJECT_NAME}-path")
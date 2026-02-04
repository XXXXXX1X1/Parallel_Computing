cmake_minimum_required(VERSION 3.16)
project(1_lab LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(TYPE "Use double of float" OFF)

add_executable(sine_sum main.cpp)

if(TYPE)
    target_compile_definitions(sine_sum PRIVATE USE_DOUBLE=1)
endif()



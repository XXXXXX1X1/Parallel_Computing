cmake_minimum_required(VERSION 3.16)
project(TaskServerProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(task_server
    main.cpp
)

add_executable(test_results
    test.cpp
)

target_include_directories(task_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(test_results PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_target(run_all
    COMMAND task_server
    COMMAND test_results
    DEPENDS task_server test_results
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
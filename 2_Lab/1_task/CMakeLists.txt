cmake_minimum_required(VERSION 3.16)
project(1_lab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(matrix matrix.cpp)

if(APPLE)
  target_compile_options(matrix PRIVATE -Xpreprocessor -fopenmp)
  target_include_directories(matrix PRIVATE /opt/homebrew/opt/libomp/include)
  target_link_directories(matrix PRIVATE /opt/homebrew/opt/libomp/lib)
  target_link_libraries(matrix PRIVATE omp)
else()
  find_package(OpenMP REQUIRED)
  target_link_libraries(matrix PRIVATE OpenMP::OpenMP_CXX)
endif()
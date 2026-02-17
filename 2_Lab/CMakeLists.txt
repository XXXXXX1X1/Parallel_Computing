cmake_minimum_required(VERSION 3.16)
project(matvec_omp CXX)

set(CMAKE_CXX_STANDARD 17)
add_executable(matvec matvec_omp.cpp)

if(APPLE)
  execute_process(
    COMMAND brew --prefix libomp
    OUTPUT_VARIABLE LIBOMP_PREFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )

  if(NOT LIBOMP_PREFIX OR NOT EXISTS "${LIBOMP_PREFIX}/include/omp.h")
    message(FATAL_ERROR "libomp not found. Run: brew install libomp")
  endif()

  target_compile_options(matvec PRIVATE -Xpreprocessor -fopenmp)
  target_include_directories(matvec PRIVATE "${LIBOMP_PREFIX}/include")
  target_link_directories(matvec PRIVATE "${LIBOMP_PREFIX}/lib")
  target_link_libraries(matvec PRIVATE omp)
  target_link_options(matvec PRIVATE "-Wl,-rpath,${LIBOMP_PREFIX}/lib")
else()
  find_package(OpenMP REQUIRED)
  target_link_libraries(matvec PRIVATE OpenMP::OpenMP_CXX)
endif()
cmake_minimum_required(VERSION 3.16)
project(matvec_omp CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BENCH "Build benchmark target" OFF)

find_package(OpenMP QUIET)

# macOS: fallback to brew libomp
if(APPLE AND NOT OpenMP_CXX_FOUND)
  execute_process(
    COMMAND brew --prefix libomp
    OUTPUT_VARIABLE LIBOMP_PREFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
  if(LIBOMP_PREFIX)
    add_library(OpenMP::OpenMP_CXX INTERFACE IMPORTED)
    target_compile_options(OpenMP::OpenMP_CXX INTERFACE
      -Xpreprocessor -fopenmp
      -I${LIBOMP_PREFIX}/include
    )
    target_link_libraries(OpenMP::OpenMP_CXX INTERFACE
      -L${LIBOMP_PREFIX}/lib -lomp
      -Wl,-rpath,${LIBOMP_PREFIX}/lib
    )
  endif()
endif()

if(NOT OpenMP_CXX_FOUND AND NOT TARGET OpenMP::OpenMP_CXX)
  message(FATAL_ERROR "OpenMP not available. macOS: brew install libomp")
endif()

if(BENCH)
  add_executable(matvec_bench matrix.cpp)
  target_link_libraries(matvec_bench PRIVATE OpenMP::OpenMP_CXX)

  add_custom_target(bench
    COMMAND $<TARGET_FILE:matvec_bench> --bench
    COMMAND python3 ${CMAKE_SOURCE_DIR}/plot.py ${CMAKE_BINARY_DIR}/results.csv
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS matvec_bench
  )
else()
  add_executable(matvec matrix.cpp)
  target_link_libraries(matvec PRIVATE OpenMP::OpenMP_CXX)

  add_custom_target(run
    COMMAND $<TARGET_FILE:matvec>
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS matvec
  )
endif()
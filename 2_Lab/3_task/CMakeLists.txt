cmake_minimum_required(VERSION 3.16)
project(lab2_omp CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

add_executable(var1 1_var.cpp)
target_link_libraries(var1 PRIVATE OpenMP::OpenMP_CXX)

add_executable(var2 2_var.cpp)
target_link_libraries(var2 PRIVATE OpenMP::OpenMP_CXX)

add_custom_target(run_var1
  COMMAND $<TARGET_FILE:var1>
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  DEPENDS var1
)

add_custom_target(run_var2
  COMMAND $<TARGET_FILE:var2>
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  DEPENDS var2
)

add_custom_target(bench
  COMMAND $<TARGET_FILE:var1> --bench
  COMMAND $<TARGET_FILE:var2> --bench
  COMMAND python3 ${CMAKE_SOURCE_DIR}/plot.py
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  DEPENDS var1 var2
)
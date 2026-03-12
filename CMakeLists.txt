cmake_minimum_required(VERSION 3.16)
project(pxhash LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

option(PXHASH_BUILD_BENCHMARKS "Build benchmark executable" ON)
option(PXHASH_BUILD_TESTS "Build test executable" ON)

if (PXHASH_BUILD_BENCHMARKS)
  find_package(benchmark QUIET)
  find_package(absl QUIET)

  if (benchmark_FOUND)
    message(STATUS "Google Benchmark found: enabling pxhash_bench")
  else()
    message(WARNING
      "PXHASH_BUILD_BENCHMARKS is ON, but Google Benchmark was not found. "
      "The pxhash_bench target will not be generated. "
      "Install Google Benchmark or reconfigure with -DPXHASH_BUILD_BENCHMARKS=OFF.")
  endif()
endif()

if (PXHASH_BUILD_BENCHMARKS AND benchmark_FOUND)
  add_executable(pxhash_bench src/main.cpp)
  target_include_directories(pxhash_bench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(pxhash_bench PRIVATE benchmark::benchmark)

  if (absl_FOUND)
    target_link_libraries(pxhash_bench PRIVATE absl::flat_hash_map)
    target_compile_definitions(pxhash_bench PRIVATE HAVE_ABSL=1)
  else()
    target_compile_definitions(pxhash_bench PRIVATE HAVE_ABSL=0)
  endif()

  target_compile_options(pxhash_bench PRIVATE
    -O3 -DNDEBUG
    -march=native
    -Wall -Wextra -Wpedantic
  )
endif()

if (PXHASH_BUILD_TESTS)
  add_executable(pxhash_tests tests/pxhash_test.cpp)
  target_include_directories(pxhash_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  target_compile_options(pxhash_tests PRIVATE
    -Wall -Wextra -Wpedantic
  )

  add_test(NAME pxhash_tests COMMAND pxhash_tests)
endif()

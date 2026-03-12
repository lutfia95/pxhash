cmake_minimum_required(VERSION 3.16)
project(pxhash_bench LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

option(PXHASH_BUILD_BENCHMARKS "Build benchmark executable" ON)

if (PXHASH_BUILD_BENCHMARKS)
  find_package(benchmark QUIET)
  find_package(absl QUIET)
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

add_executable(pxhash_tests tests/pxhash_test.cpp)
target_include_directories(pxhash_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_options(pxhash_tests PRIVATE
  -Wall -Wextra -Wpedantic
)

add_test(NAME pxhash_tests COMMAND pxhash_tests)

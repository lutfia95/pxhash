cmake_minimum_required(VERSION 3.15)

project(PXHASH LANGUAGES CXX)

include(FetchContent)

# -----------------------------------
# Setup C++ Standard & Optimizations
# -----------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-O3 -march=native -mtune=native -funroll-loops -flto)
    add_link_options(-flto)
elseif (MSVC)
    add_compile_options(/O2 /arch:AVX2)
endif()

# -----------------------------------
# Fetch Google Benchmark
# -----------------------------------
set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
set(BENCHMARK_ENABLE_TESTING OFF)
set(BENCHMARK_DOWNLOAD_DEPENDENCIES OFF)

FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.8.3
)
FetchContent_MakeAvailable(benchmark)

# -----------------------------------
# Fetch Abseil
# -----------------------------------
FetchContent_Declare(
    abseil
    GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
    GIT_TAG 20230125.2
)
FetchContent_MakeAvailable(abseil)

# -----------------------------------
# Fetch Folly (optional but recommended)
# -----------------------------------
FetchContent_Declare(
    folly
    GIT_REPOSITORY https://github.com/facebook/folly.git
    GIT_TAG v2023.03.20.00
)
FetchContent_MakeAvailable(folly)

# -----------------------------------
# SIMD detection macros
# -----------------------------------
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-mavx512f" COMPILER_SUPPORTS_AVX512)
check_cxx_compiler_flag("-mavx2" COMPILER_SUPPORTS_AVX2)

if(COMPILER_SUPPORTS_AVX512)
    add_compile_definitions(HAVE_AVX512=1)
elseif(COMPILER_SUPPORTS_AVX2)
    add_compile_definitions(HAVE_AVX2=1)
else()
    message(WARNING "No AVX512/AVX2 support detected. Falling back to SSE2.")
endif()

# -----------------------------------
# PXHASH Library (header-only)
# -----------------------------------
add_library(pxhash INTERFACE)
target_include_directories(pxhash INTERFACE src)

# -----------------------------------
# PXHASH Benchmarks Executable
# -----------------------------------
add_executable(pxhash_benchmarks
    src/main.cpp
)

target_include_directories(pxhash_benchmarks PRIVATE src)

target_link_libraries(pxhash_benchmarks
    pxhash
    benchmark::benchmark
    absl::base
    absl::flat_hash_map
    folly
)

# -----------------------------------
# Warnings (optional)
# -----------------------------------
# add_compile_options(-Wall -Wextra -Wpedantic)

message(STATUS "PXHASH build complete!")

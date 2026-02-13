#include <iostream>
#include <unordered_map>
#include <vector>
#include <random>
#include <thread>

#include "pxhash.hpp"

#include <benchmark/benchmark.h>

#if __has_include("absl/container/flat_hash_map.h")
  #include "absl/container/flat_hash_map.h"
  #define HAVE_ABSL 1
#else
  #define HAVE_ABSL 0
#endif

constexpr size_t TOTAL_ITEMS = 1'000'000;

static size_t nextPowerOfTwo(size_t n) {
  if (n == 0) return 1;
  n--;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
#if INTPTR_MAX == INT64_MAX
  n |= n >> 32;
#endif
  return n + 1;
}

static void printPXHashLogo() {
  std::cout << R"(

            ░▒▓███████▓▒░  ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░  ░▒▓██████▓▒░   ░▒▓███████▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ 
            ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░        ░▒▓█▓▒░░▒▓█▓▒░ 
            ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░        ░▒▓█▓▒░░▒▓█▓▒░ 
            ░▒▓███████▓▒░   ░▒▓██████▓▒░  ░▒▓████████▓▒░ ░▒▓████████▓▒░  ░▒▓██████▓▒░  ░▒▓████████▓▒░ 
            ░▒▓█▓▒░        ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░        ░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ 
            ░▒▓█▓▒░        ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░        ░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ 
            ░▒▓█▓▒░        ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓███████▓▒░  ░▒▓█▓▒░░▒▓█▓▒░ 
                                                                                                                                                                                                                                                            
                            High-Performance Hash Table, by Ah Lu 

)" << std::endl;

  std::cout << "Running on " << std::thread::hardware_concurrency() << " Threads\n";
  std::cout << "------------------------------------------\n\n";
}

static std::vector<uint64_t> generateKeys(size_t n) {
  std::vector<uint64_t> keys(n);
  std::mt19937_64 rng(12345);
  std::uniform_int_distribution<uint64_t> dist;
  for (size_t i = 0; i < n; ++i) keys[i] = dist(rng);
  return keys;
}

static std::vector<uint64_t> testKeys = generateKeys(TOTAL_ITEMS);

static void BM_PXHash_Insert(benchmark::State& state) {
  for (auto _ : state) {
    pxhash::PXHash<uint64_t, uint64_t> map(nextPowerOfTwo(TOTAL_ITEMS * 2));
    for (size_t i = 0; i < (size_t)state.range(0); ++i) map.insert(testKeys[i], testKeys[i]);
    benchmark::DoNotOptimize(map);
  }
}
BENCHMARK(BM_PXHash_Insert)->Arg(TOTAL_ITEMS);

static void BM_PXHash_Find(benchmark::State& state) {
  pxhash::PXHash<uint64_t, uint64_t> map(nextPowerOfTwo(TOTAL_ITEMS));
  for (size_t i = 0; i < (size_t)state.range(0); ++i) map.insert(testKeys[i], testKeys[i]);

  uint64_t found = 0;
  for (auto _ : state) {
    for (size_t i = 0; i < (size_t)state.range(0); ++i) {
      uint64_t val;
      if (map.find(testKeys[i], val)) found++;
    }
    benchmark::DoNotOptimize(found);
  }
}
BENCHMARK(BM_PXHash_Find)->Arg(TOTAL_ITEMS);

static void BM_StdMap_Insert(benchmark::State& state) {
  for (auto _ : state) {
    std::unordered_map<uint64_t, uint64_t> map;
    map.reserve((size_t)state.range(0));
    for (size_t i = 0; i < (size_t)state.range(0); ++i) map.emplace(testKeys[i], testKeys[i]);
    benchmark::DoNotOptimize(map);
  }
}
BENCHMARK(BM_StdMap_Insert)->Arg(TOTAL_ITEMS);

static void BM_StdMap_Find(benchmark::State& state) {
  std::unordered_map<uint64_t, uint64_t> map;
  map.reserve((size_t)state.range(0));
  for (size_t i = 0; i < (size_t)state.range(0); ++i) map.emplace(testKeys[i], testKeys[i]);

  uint64_t found = 0;
  for (auto _ : state) {
    for (size_t i = 0; i < (size_t)state.range(0); ++i) {
      auto it = map.find(testKeys[i]);
      if (it != map.end()) found++;
    }
    benchmark::DoNotOptimize(found);
  }
}
BENCHMARK(BM_StdMap_Find)->Arg(TOTAL_ITEMS);

#if HAVE_ABSL
static void BM_AbslMap_Insert(benchmark::State& state) {
  for (auto _ : state) {
    absl::flat_hash_map<uint64_t, uint64_t> map;
    map.reserve((size_t)state.range(0));
    for (size_t i = 0; i < (size_t)state.range(0); ++i) map.emplace(testKeys[i], testKeys[i]);
    benchmark::DoNotOptimize(map);
  }
}
BENCHMARK(BM_AbslMap_Insert)->Arg(TOTAL_ITEMS);

static void BM_AbslMap_Find(benchmark::State& state) {
  absl::flat_hash_map<uint64_t, uint64_t> map;
  map.reserve((size_t)state.range(0));
  for (size_t i = 0; i < (size_t)state.range(0); ++i) map.emplace(testKeys[i], testKeys[i]);

  uint64_t found = 0;
  for (auto _ : state) {
    for (size_t i = 0; i < (size_t)state.range(0); ++i) {
      auto it = map.find(testKeys[i]);
      if (it != map.end()) found++;
    }
    benchmark::DoNotOptimize(found);
  }
}
BENCHMARK(BM_AbslMap_Find)->Arg(TOTAL_ITEMS);
#endif

int main(int argc, char** argv) {
  ::benchmark::Initialize(&argc, argv);
  printPXHashLogo();
  ::benchmark::RunSpecifiedBenchmarks();
  return 0;
}

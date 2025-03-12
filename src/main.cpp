#include <iostream>
#include <unordered_map>
#include <vector>
#include <random>
#include <thread>
#include <chrono>

// PXHASH header
#include "pxhash.hpp"

// Google Benchmark
#include <benchmark/benchmark.h>

// Abseil and Folly maps (include if installed)
#include "absl/container/flat_hash_map.h"
#include "folly/container/F14Map.h"

constexpr size_t TOTAL_ITEMS = 1'000'000;

size_t nextPowerOfTwo(size_t n) {
    if (n == 0) return 1;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    return n + 1;
}

void printPXHashLogo() {
    std::cout << R"(

            ░▒▓███████▓▒░  ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░  ░▒▓██████▓▒░   ░▒▓███████▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ 
            ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░        ░▒▓█▓▒░░▒▓█▓▒░ 
            ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░        ░▒▓█▓▒░░▒▓█▓▒░ 
            ░▒▓███████▓▒░   ░▒▓██████▓▒░  ░▒▓████████▓▒░ ░▒▓████████▓▒░  ░▒▓██████▓▒░  ░▒▓████████▓▒░ 
            ░▒▓█▓▒░        ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░        ░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ 
            ░▒▓█▓▒░        ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░        ░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ 
            ░▒▓█▓▒░        ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓███████▓▒░  ░▒▓█▓▒░░▒▓█▓▒░ 
                                                                                                                                                                                                                                                            
                            High-Performance Concurrent Hash Table, by Ah Lu 

)" << std::endl;

    std::cout << "Running on " << std::thread::hardware_concurrency() << " Threads\n";
    std::cout << "------------------------------------------" << std::endl;
    std::cout << std::endl;
}


// Helper to generate test data
std::vector<uint64_t> generateKeys(size_t n) {
    std::vector<uint64_t> keys(n);
    std::mt19937_64 rng(12345);
    std::uniform_int_distribution<uint64_t> dist;
    for (size_t i = 0; i < n; ++i) {
        keys[i] = dist(rng);
    }
    return keys;
}

static std::vector<uint64_t> testKeys = generateKeys(TOTAL_ITEMS);

// PXHASH Benchmarks
static void BM_PXHash_Insert(benchmark::State& state) {
    for (auto _ : state) {
        pxhash::PXHash<uint64_t, uint64_t> map(nextPowerOfTwo(TOTAL_ITEMS * 2));
        for (size_t i = 0; i < state.range(0); ++i) {
            map.insert(testKeys[i], testKeys[i]);
        }
        benchmark::DoNotOptimize(map);
    }
}
BENCHMARK(BM_PXHash_Insert)->Arg(TOTAL_ITEMS);

static void BM_PXHash_Find(benchmark::State& state) {
    pxhash::PXHash<uint64_t, uint64_t> map(nextPowerOfTwo(TOTAL_ITEMS));
    for (size_t i = 0; i < state.range(0); ++i) {
        map.insert(testKeys[i], testKeys[i]);
    }

    uint64_t found = 0;
    for (auto _ : state) {
        for (size_t i = 0; i < state.range(0); ++i) {
            uint64_t val;
            if (map.find(testKeys[i], val)) {
                found++;
            }
        }
        benchmark::DoNotOptimize(found);
    }
}
BENCHMARK(BM_PXHash_Find)->Arg(TOTAL_ITEMS);

// std::unordered_map Benchmarks
static void BM_StdMap_Insert(benchmark::State& state) {
    for (auto _ : state) {
        std::unordered_map<uint64_t, uint64_t> map;
        for (size_t i = 0; i < state.range(0); ++i) {
            map.emplace(testKeys[i], testKeys[i]);
        }
        benchmark::DoNotOptimize(map);
    }
}
BENCHMARK(BM_StdMap_Insert)->Arg(TOTAL_ITEMS);

static void BM_StdMap_Find(benchmark::State& state) {
    std::unordered_map<uint64_t, uint64_t> map;
    for (size_t i = 0; i < state.range(0); ++i) {
        map.emplace(testKeys[i], testKeys[i]);
    }

    uint64_t found = 0;
    for (auto _ : state) {
        for (size_t i = 0; i < state.range(0); ++i) {
            auto it = map.find(testKeys[i]);
            if (it != map.end()) {
                found++;
            }
        }
        benchmark::DoNotOptimize(found);
    }
}
BENCHMARK(BM_StdMap_Find)->Arg(TOTAL_ITEMS);

// Abseil flat_hash_map Benchmarks
static void BM_AbslMap_Insert(benchmark::State& state) {
    for (auto _ : state) {
        absl::flat_hash_map<uint64_t, uint64_t> map;
        for (size_t i = 0; i < state.range(0); ++i) {
            map.emplace(testKeys[i], testKeys[i]);
        }
        benchmark::DoNotOptimize(map);
    }
}
BENCHMARK(BM_AbslMap_Insert)->Arg(TOTAL_ITEMS);

static void BM_AbslMap_Find(benchmark::State& state) {
    absl::flat_hash_map<uint64_t, uint64_t> map;
    for (size_t i = 0; i < state.range(0); ++i) {
        map.emplace(testKeys[i], testKeys[i]);
    }

    uint64_t found = 0;
    for (auto _ : state) {
        for (size_t i = 0; i < state.range(0); ++i) {
            auto it = map.find(testKeys[i]);
            if (it != map.end()) {
                found++;
            }
        }
        benchmark::DoNotOptimize(found);
    }
}
BENCHMARK(BM_AbslMap_Find)->Arg(TOTAL_ITEMS);

// Folly F14 Map Benchmarks
static void BM_F14Map_Insert(benchmark::State& state) {
    for (auto _ : state) {
        folly::F14FastMap<uint64_t, uint64_t> map;
        for (size_t i = 0; i < state.range(0); ++i) {
            map.emplace(testKeys[i], testKeys[i]);
        }
        benchmark::DoNotOptimize(map);
    }
}
BENCHMARK(BM_F14Map_Insert)->Arg(TOTAL_ITEMS);

static void BM_F14Map_Find(benchmark::State& state) {
    folly::F14FastMap<uint64_t, uint64_t> map;
    for (size_t i = 0; i < state.range(0); ++i) {
        map.emplace(testKeys[i], testKeys[i]);
    }

    uint64_t found = 0;
    for (auto _ : state) {
        for (size_t i = 0; i < state.range(0); ++i) {
            auto it = map.find(testKeys[i]);
            if (it != map.end()) {
                found++;
            }
        }
        benchmark::DoNotOptimize(found);
    }
}
BENCHMARK(BM_F14Map_Find)->Arg(TOTAL_ITEMS);

int main(int argc, char** argv) {
    ::benchmark::Initialize(&argc, argv);
    printPXHashLogo();
    ::benchmark::RunSpecifiedBenchmarks();
    return 0;
}

// Main function
// BENCHMARK_MAIN();

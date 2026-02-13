# PXHash (Work in Progress)
High-Performance Concurrent Hash Table
It is designed to compete with:

- `std::unordered_map`
- `absl::flat_hash_map`
- `folly::F14FastMap`

# Requirements

## Minimum

- C++20
- GCC ≥ 10 or Clang ≥ 12
- CMake ≥ 3.16

## For benchmarking (optional but recommended)

- Google Benchmark
- Abseil
- Folly

# Installation

## Ubuntu / Debian
### Core toolchain

```
sudo apt update
sudo apt install -y build-essential cmake pkg-config
sudo apt install -y libbenchmark-dev
sudo apt install -y libabsl-dev

```

# Build
```
git clone https://github.com/lutfia95/pxhash.git
cd pxhash
mkdir build
cd build
cmake ..
make -j$(nproc)
```
# Benchmarking
```
./pxhash_bench

```
# Tuning
## Enable AVX2 explicitly
```
target_compile_options(pxhash_bench PRIVATE -mavx2)

```
Or use: 
```
-march=native

```
## Remove RTTI / exceptions (smaller binary)
```
-fno-exceptions
-fno-rtti

```
# Usage Example
```
#include <iostream>
#include <string>
#include "pxhash.hpp"

int main() {
    // PXHash<Key, Value>
    // Here: Key is std::string (e.g. genomic coordinate / identifier)
    //       Value is uint64_t (e.g. a count, ID, offset, etc.)
    pxhash::PXHash<std::string, uint64_t> map;

    // Insert a key-value pair:
    //   key   = "chr1:12345"
    //   value = 100
    //
    // Conceptually:
    //   map["chr1:12345"] = 100
    map.insert("chr1:12345", 100);

    // Insert another key-value pair.
    // Using std::string explicitly here (same thing, just showing the type).
    //   key   = "chr2:999"
    //   value = 200
    map.insert(std::string("chr2:999"), 200);

    // Look up a key:
    // If the key exists, PXHash writes the stored value into `value` and returns true.
    uint64_t value = 0;
    if (map.find("chr1:12345", value)) {
        // If found, `value` now contains the value stored for that key.
        // Here it should print 100.
        std::cout << "found chr1:12345 => " << value << "\n";
    } else {
        std::cout << "chr1:12345 not found\n";
    }

    // Erase removes the key-value pair for that key (if it exists).
    // After this, "chr1:12345" should no longer be in the map.
    map.erase("chr1:12345");

    // Confirm it's gone.
    if (!map.find("chr1:12345", value)) {
        std::cout << "after erase, chr1:12345 not found\n";
    }

    return 0;
}


```

```

            ░▒▓███████▓▒░  ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░  ░▒▓██████▓▒░   ░▒▓███████▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ 
            ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░        ░▒▓█▓▒░░▒▓█▓▒░ 
            ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░        ░▒▓█▓▒░░▒▓█▓▒░ 
            ░▒▓███████▓▒░   ░▒▓██████▓▒░  ░▒▓████████▓▒░ ░▒▓████████▓▒░  ░▒▓██████▓▒░  ░▒▓████████▓▒░ 
            ░▒▓█▓▒░        ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░        ░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ 
            ░▒▓█▓▒░        ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░        ░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ 
            ░▒▓█▓▒░        ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓███████▓▒░  ░▒▓█▓▒░░▒▓█▓▒░ 
                                                                                                                                                                                                                                                            
                            High-Performance Hash Table, by Ah Lu 


Running on 12 Threads
------------------------------------------

2026-02-13T15:15:14+01:00
Running ./pxhash_bench
Run on (12 X 3900 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x6)
  L1 Instruction 32 KiB (x6)
  L2 Unified 512 KiB (x6)
  L3 Unified 16384 KiB (x1)
Load Average: 0.44, 1.13, 1.02
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
***WARNING*** Library was built as DEBUG. Timings may be affected.
--------------------------------------------------------------------
Benchmark                          Time             CPU   Iterations
--------------------------------------------------------------------
BM_PXHash_Insert/1000000    27190001 ns     27188908 ns           26
BM_PXHash_Find/1000000      10375694 ns     10374461 ns           67
BM_StdMap_Insert/1000000   196319636 ns    196251472 ns            4
BM_StdMap_Find/1000000      24233274 ns     24232182 ns           27
BM_AbslMap_Insert/1000000   20795273 ns     20794570 ns           34
BM_AbslMap_Find/1000000     18180487 ns     18179385 ns           38


```
# License
MIT
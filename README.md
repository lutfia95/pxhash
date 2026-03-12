# PXHash
High-Performance Concurrent Hash Table

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

# Container / Docker
```bash
docker build -t pxhash .
docker run --rm pxhash
```

Run the benchmark binary inside the container:

```bash
docker run --rm pxhash ./build/pxhash_bench
```

## Usage Example
```cpp
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

## Binary Persistence

`PXHash` can save to and load from a compact binary file when both `KeyType` and `ValueType` are trivially copyable, for example `uint64_t`, POD structs, or fixed-size IDs.

```cpp
#include <cstdint>
#include "pxhash.hpp"

int main() {
    pxhash::PXHash<std::uint64_t, std::uint64_t> map;
    map.insert(10, 100);
    map.insert(20, 200);

    map.saveBinary("table.pxh");

    pxhash::PXHash<std::uint64_t, std::uint64_t> restored;
    restored.loadBinary("table.pxh");
}
```

Notes:

- The binary format is a small PXHash-specific header plus raw key/value records.
- This path intentionally rejects non-trivially-copyable types such as `std::string`.
- The file is intended for use on compatible builds and architectures; it is not a cross-platform interchange format.

## Tuning

Enable AVX2 explicitly:

```cmake
target_compile_options(pxhash_bench PRIVATE -mavx2)
```

Or use:

```text
-march=native
```

Remove RTTI / exceptions for a smaller binary:

```text
-fno-exceptions
-fno-rtti
```

## License

MIT

# PXHash (Work in Progress)
High-Performance Concurrent Hash Table


# Installation

```
# Core tools
sudo apt-get update
sudo apt-get install -y cmake g++ libpthread-stubs0-dev build-essential

# Google Benchmark
sudo apt-get install -y libbenchmark-dev

# Abseil C++ (flat_hash_map)
sudo apt-get install -y libabsl-dev
sudo apt install -y libfast-float-dev

# Folly (Facebook's library)
sudo apt-get install -y libdouble-conversion-dev libboost-all-dev \
                        libevent-dev libgflags-dev libgoogle-glog-dev \
                        libiberty-dev liblz4-dev liblzma-dev libsnappy-dev \
                        make zlib1g-dev binutils-dev libjemalloc-dev \
                        libssl-dev pkg-config libunwind-dev
                        
git clone https://github.com/facebook/folly.git
cd folly
mkdir _build && cd _build
cmake ..
make -j$(nproc)
sudo make install

```
```
git clone https://github.com/lutfia95/pxhash.git
cd pxhash
mkdir build && cd build
cmake ..
make -j$(nproc)
```
```

            ░▒▓███████▓▒░  ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░  ░▒▓██████▓▒░   ░▒▓███████▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ 
            ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░        ░▒▓█▓▒░░▒▓█▓▒░ 
            ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░        ░▒▓█▓▒░░▒▓█▓▒░ 
            ░▒▓███████▓▒░   ░▒▓██████▓▒░  ░▒▓████████▓▒░ ░▒▓████████▓▒░  ░▒▓██████▓▒░  ░▒▓████████▓▒░ 
            ░▒▓█▓▒░        ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░        ░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ 
            ░▒▓█▓▒░        ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░        ░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ 
            ░▒▓█▓▒░        ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓███████▓▒░  ░▒▓█▓▒░░▒▓█▓▒░ 
                                                                                                                                                                                                                                                            
                            High-Performance Concurrent Hash Table, by Ah Lu 


Running on 12 Threads
------------------------------------------

2025-03-12T12:12:57+01:00
Running ./pxhash_benchmarks
Run on (12 X 3900 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x6)
  L1 Instruction 32 KiB (x6)
  L2 Unified 512 KiB (x6)
  L3 Unified 16384 KiB (x1)
Load Average: 2.89, 1.57, 1.17
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
***WARNING*** Library was built as DEBUG. Timings may be affected.
--------------------------------------------------------------------
Benchmark                          Time             CPU   Iterations
--------------------------------------------------------------------
BM_PXHash_Insert/1000000    62882366 ns     62880940 ns           10
BM_PXHash_Find/1000000     111489330 ns    111456308 ns            6
BM_StdMap_Insert/1000000   348376680 ns    348367021 ns            2
BM_StdMap_Find/1000000      29548402 ns     29454086 ns           26
BM_AbslMap_Insert/1000000   76280066 ns     76273990 ns            9
BM_AbslMap_Find/1000000     20748918 ns     20749120 ns           35
BM_F14Map_Insert/1000000    59497625 ns     59485756 ns           11
BM_F14Map_Find/1000000      24217556 ns     24217163 ns           27

```

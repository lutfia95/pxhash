#ifndef PXHASH_HPP
#define PXHASH_HPP

#include <atomic>
#include <vector>
#include <memory>
#include <immintrin.h>
#include <thread>
#include <functional>
#include <mutex>
#include <cassert>
#include <cstring>
#include <shared_mutex>
#include <chrono>

// Architecture check and settings
#if defined(__AVX512F__)
#define PXHASH_SIMD 512
#elif defined(__AVX2__)
#define PXHASH_SIMD 256
#else
#define PXHASH_SIMD 128
#endif

namespace pxhash {

constexpr uint8_t EMPTY = 0x80;
constexpr uint8_t TOMBSTONE = 0xFE;

// Epoch-based reclamation for safe memory release
class EpochManager {
public:
    void enter() { current_epoch.fetch_add(1, std::memory_order_acq_rel); }
    void exit() { current_epoch.fetch_sub(1, std::memory_order_acq_rel); }
    bool no_readers() { return current_epoch.load(std::memory_order_acquire) == 0; }

private:
    std::atomic<size_t> current_epoch{0};
};

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

template<typename KeyType, typename ValueType, typename Hash = std::hash<KeyType>>
class PXHash {
private:
    static constexpr size_t GROUP_SIZE = 16; // Group of slots scanned together
    static constexpr size_t MAX_LOAD = 60;   // 60% load factor threshold before resize

    struct alignas(64) Bucket {
        std::atomic<uint8_t> ctrl;
        KeyType key;
        ValueType value;

        Bucket() : ctrl(EMPTY), key(), value() {}

        Bucket(const Bucket& other)
            : ctrl(other.ctrl.load(std::memory_order_relaxed)),
            key(other.key),
            value(other.value) {}

        Bucket(Bucket&& other) noexcept
            : ctrl(other.ctrl.load(std::memory_order_relaxed)),
            key(std::move(other.key)),
            value(std::move(other.value)) {}

        Bucket& operator=(const Bucket& other) {
            ctrl.store(other.ctrl.load(std::memory_order_relaxed), std::memory_order_relaxed);
            key = other.key;
            value = other.value;
            return *this;
        }

        Bucket& operator=(Bucket&& other) noexcept {
            ctrl.store(other.ctrl.load(std::memory_order_relaxed), std::memory_order_relaxed);
            key = std::move(other.key);
            value = std::move(other.value);
            return *this;
        }
    };


    struct Table {
        size_t capacity;
        std::vector<Bucket> buckets;
        std::atomic<size_t> size;

        Table(size_t cap) : capacity(cap), size(0) {
            if (capacity == 0 || (capacity & (capacity - 1)) != 0) {
                std::cerr << "Invalid capacity: " << capacity << std::endl;
                throw std::runtime_error("Capacity must be power of 2 and > 0");
            }
            buckets.resize(capacity);
        }
    };

    std::atomic<Table*> currentTable;
    std::mutex resize_mutex;
    EpochManager epoch;

    Hash hasher;
    std::atomic<bool> resizing{false};

    uint8_t getFingerprint(size_t hash) const {
        return (hash >> 56) & 0x7F;  // 7-bit fingerprint
    }

    size_t indexForHash(size_t hash, size_t cap) const {
        return hash & (cap - 1);
    }

    void startResize() {
        std::lock_guard<std::mutex> lock(resize_mutex);

        if (resizing.load(std::memory_order_acquire)) return;
        resizing.store(true, std::memory_order_release);

        Table* old_table = currentTable.load(std::memory_order_acquire);
        
        size_t new_capacity = old_table->capacity * 2;
        new_capacity = nextPowerOfTwo(new_capacity);
        std::cout << "[PXHASH] Resizing from " << old_table->capacity << " to " << new_capacity << std::endl;

        Table* new_table = new Table(new_capacity);

        // Migrate buckets from old_table to new_table
        for (size_t i = 0; i < old_table->capacity; ++i) {
            auto& old_bucket = old_table->buckets[i];
            uint8_t ctrl = old_bucket.ctrl.load(std::memory_order_acquire);

            if (ctrl != EMPTY && ctrl != TOMBSTONE) {
                insertInternal(new_table, old_bucket.key, old_bucket.value);
            }
        }

        currentTable.store(new_table, std::memory_order_release);
        resizing.store(false, std::memory_order_release);

        while (!epoch.no_readers()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        delete old_table;
    }


    void insertInternal(Table* table, const KeyType& key, const ValueType& value) {
        size_t hash = hasher(key);
        uint8_t fingerprint = getFingerprint(hash);
        size_t index = indexForHash(hash, table->capacity);

        for (size_t i = 0; i < table->capacity; ++i) {
            auto& bucket = table->buckets[index];
            uint8_t ctrl = bucket.ctrl.load(std::memory_order_acquire);

            if (ctrl == EMPTY || ctrl == TOMBSTONE) {
                uint8_t expected = ctrl;
                if (bucket.ctrl.compare_exchange_strong(expected, fingerprint, std::memory_order_acq_rel)) {
                    bucket.key = key;
                    bucket.value = value;
                    table->size.fetch_add(1, std::memory_order_relaxed);
                    return;
                }
            } else if (bucket.key == key) {
                bucket.value = value;
                return;
            }

            index = (index + 1) & (table->capacity - 1);
        }

        throw std::runtime_error("Insert failed! Table is full.");
    }

public:
    explicit PXHash(size_t initial_capacity = 1024) {
        Table* table = new Table(initial_capacity);
        currentTable.store(table, std::memory_order_release);
    }

    ~PXHash() {
        Table* table = currentTable.load(std::memory_order_acquire);
        delete table;
    }

    void insert(const KeyType& key, const ValueType& value) {
        epoch.enter();

        Table* table = currentTable.load(std::memory_order_acquire);

        // Trigger resize BEFORE insert fails
        while (((table->size.load(std::memory_order_relaxed) * 100) / table->capacity) >= MAX_LOAD) {
            epoch.exit();   // exit before resizing
            startResize();  // perform resize
            epoch.enter();
            table = currentTable.load(std::memory_order_acquire);
        }

        insertInternal(table, key, value);

        epoch.exit();
    }

    bool find(const KeyType& key, ValueType& out_value) {
        epoch.enter();

        Table* table = currentTable.load(std::memory_order_acquire);
        size_t hash = hasher(key);
        uint8_t fingerprint = getFingerprint(hash);
        size_t index = indexForHash(hash, table->capacity);

        bool found = false;

#if PXHASH_SIMD >= 256
        __m256i fingerprint_vec = _mm256_set1_epi8(static_cast<char>(fingerprint));

        for (size_t group = 0; group < table->capacity / GROUP_SIZE; ++group) {
            uint8_t ctrl_bytes[GROUP_SIZE];
            for (size_t i = 0; i < GROUP_SIZE; ++i) {
                ctrl_bytes[i] = table->buckets[(index + i) & (table->capacity - 1)].ctrl.load();
            }

            __m256i ctrl_vec = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(ctrl_bytes));
            __m256i cmp = _mm256_cmpeq_epi8(ctrl_vec, fingerprint_vec);
            int mask = _mm256_movemask_epi8(cmp);

            while (mask != 0) {
                int i = __builtin_ctz(mask);
                size_t actual_index = (index + i) & (table->capacity - 1);
                auto& bucket = table->buckets[actual_index];

                if (bucket.key == key) {
                    out_value = bucket.value;
                    found = true;
                    break;
                }

                mask &= (mask - 1);
            }

            if (found) break;

            index = (index + GROUP_SIZE) & (table->capacity - 1);
        }
#else
        for (size_t i = 0; i < table->capacity; ++i) {
            auto& bucket = table->buckets[index];
            uint8_t ctrl = bucket.ctrl.load(std::memory_order_acquire);

            if (ctrl == EMPTY) break;

            if (ctrl == fingerprint && bucket.key == key) {
                out_value = bucket.value;
                found = true;
                break;
            }

            index = (index + 1) & (table->capacity - 1);
        }
#endif

        epoch.exit();
        return found;
    }

    bool erase(const KeyType& key) {
        epoch.enter();

        Table* table = currentTable.load(std::memory_order_acquire);
        size_t hash = hasher(key);
        uint8_t fingerprint = getFingerprint(hash);
        size_t index = indexForHash(hash, table->capacity);

        for (size_t i = 0; i < table->capacity; ++i) {
            auto& bucket = table->buckets[index];
            uint8_t ctrl = bucket.ctrl.load(std::memory_order_acquire);

            if (ctrl == EMPTY) break;

            if (ctrl == fingerprint && bucket.key == key) {
                bucket.ctrl.store(TOMBSTONE, std::memory_order_release);
                table->size.fetch_sub(1, std::memory_order_relaxed);
                epoch.exit();
                return true;
            }

            index = (index + 1) & (table->capacity - 1);
        }

        epoch.exit();
        return false;
    }

    size_t size() const {
        Table* table = currentTable.load(std::memory_order_acquire);
        return table->size.load(std::memory_order_relaxed);
    }
};

}  // namespace pxhash

#endif  // PXHASH_HPP

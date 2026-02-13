#ifndef PXHASH_HPP
#define PXHASH_HPP

#include <cstdint>
#include <cstdlib>
#include <functional>
#include <immintrin.h>
#include <type_traits>
#include <utility>
#include <vector>

namespace pxhash {

// SwissTable-style control bytes
static constexpr uint8_t EMPTY   = 0x80; // high bit set
static constexpr uint8_t DELETED = 0xFE;

#if defined(__AVX2__)
static constexpr size_t GROUP_SIZE = 32;
#else
static constexpr size_t GROUP_SIZE = 16;
#endif

static inline size_t alignUp(size_t n, size_t a) { return (n + (a - 1)) & ~(a - 1); }

static inline size_t nextPowerOfTwo(size_t n) {
  if (n <= 1) return 1;
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

static inline uint8_t h2_from_hash(size_t h) {
  // 7-bit fingerprint from top bits
  return static_cast<uint8_t>((h >> (sizeof(size_t) * 8 - 7)) & 0x7F);
}

template <class K, class V>
struct Slot {
  K key;
  V value;
};

template <typename KeyType, typename ValueType, typename Hash = std::hash<KeyType>,
          typename Eq = std::equal_to<KeyType>>
class PXHash {
public:
  explicit PXHash(size_t initial_capacity = 0) : hasher_(), eq_() {
    if (initial_capacity) reserve(initial_capacity);
  }

  PXHash(const PXHash&) = delete;
  PXHash& operator=(const PXHash&) = delete;

  PXHash(PXHash&&) noexcept = default;
  PXHash& operator=(PXHash&&) noexcept = default;

  size_t size() const noexcept { return size_; }
  bool empty() const noexcept { return size_ == 0; }

  void reserve(size_t n) {
    // target max load ~ 7/8
    size_t need = (n * 8) / 7 + 1;
    size_t cap = nextPowerOfTwo(need);
    if (cap < minCapacity()) cap = minCapacity();
    cap = alignUp(cap, GROUP_SIZE);
    if (cap <= capacity_) return;
    rehash(cap);
  }

  void insert(const KeyType& key, const ValueType& value) {
    maybeGrowForInsert();
    insertOrAssignImpl(key, value);
  }

  void insert(KeyType&& key, ValueType&& value) {
    maybeGrowForInsert();
    insertOrAssignImpl(std::move(key), std::move(value));
  }

  bool find(const KeyType& key, ValueType& out_value) const {
    if (capacity_ == 0) return false;

    size_t h = hasher_(key);
    uint8_t h2 = h2_from_hash(h);
    size_t idx = h & mask_;

    for (;;) {
      const uint8_t* base = ctrl_.data() + idx;

      uint32_t m = matchH2Mask(base, h2);
      while (m) {
        unsigned bit = ctz(m);
        size_t pos = idx + bit;
        const auto& s = slots_[pos];
        if (eq_(s.key, key)) {
          out_value = s.value;
          return true;
        }
        m &= (m - 1);
      }

      if (emptyMask(base)) return false;
      idx = (idx + GROUP_SIZE) & mask_;
    }
  }

  bool erase(const KeyType& key) {
    if (capacity_ == 0) return false;

    size_t h = hasher_(key);
    uint8_t h2 = h2_from_hash(h);
    size_t idx = h & mask_;

    for (;;) {
      uint8_t* base = ctrl_.data() + idx;

      uint32_t m = matchH2Mask(base, h2);
      while (m) {
        unsigned bit = ctz(m);
        size_t pos = idx + bit;
        if (ctrl_[pos] == h2 && eq_(slots_[pos].key, key)) {
          setCtrl(pos, DELETED);
          ++deleted_;
          --size_;
          if (deleted_ > (capacity_ >> 2)) rehash(capacity_);
          return true;
        }
        m &= (m - 1);
      }

      if (emptyMask(base)) return false;
      idx = (idx + GROUP_SIZE) & mask_;
    }
  }

private:
  static constexpr size_t minCapacity() { return GROUP_SIZE * 2; }
  static constexpr size_t kNumer = 7;
  static constexpr size_t kDenom = 8;

  Hash hasher_;
  Eq eq_;

  size_t capacity_{0};
  size_t mask_{0};
  size_t size_{0};
  size_t deleted_{0};

  // ctrl_ has capacity_ + GROUP_SIZE bytes (tail mirrors first GROUP_SIZE)
  std::vector<uint8_t> ctrl_;
  std::vector<Slot<KeyType, ValueType>> slots_;

  static inline unsigned ctz(uint32_t x) {
#if defined(_MSC_VER)
    unsigned long idx;
    _BitScanForward(&idx, x);
    return (unsigned)idx;
#else
    return (unsigned)__builtin_ctz(x);
#endif
  }

  inline void setCtrl(size_t pos, uint8_t v) noexcept {
    ctrl_[pos] = v;
    if (pos < GROUP_SIZE) ctrl_[pos + capacity_] = v; // mirror
  }

  void initTable(size_t cap) {
    capacity_ = cap;
    mask_ = cap - 1;
    size_ = 0;
    deleted_ = 0;

    ctrl_.assign(capacity_ + GROUP_SIZE, EMPTY);
    slots_.resize(capacity_ + GROUP_SIZE);
    for (size_t i = 0; i < GROUP_SIZE; ++i) ctrl_[capacity_ + i] = ctrl_[i];
  }

  void rehash(size_t newCap) {
    PXHash tmp;
    tmp.initTable(newCap);

    if (capacity_) {
      for (size_t i = 0; i < capacity_; ++i) {
        uint8_t c = ctrl_[i];
        if (c != EMPTY && c != DELETED) {
          tmp.insertOrAssignImpl(std::move(slots_[i].key), std::move(slots_[i].value));
        }
      }
    }
    *this = std::move(tmp);
  }

  void maybeGrowForInsert() {
    if (capacity_ == 0) {
      initTable(minCapacity());
      return;
    }
    size_t used = size_ + deleted_;
    if (used * kDenom >= capacity_ * kNumer) {
      if (deleted_ > (capacity_ >> 3)) rehash(capacity_);
      else rehash(capacity_ * 2);
    }
  }

  static inline uint32_t matchH2Mask(const uint8_t* base, uint8_t h2) {
#if defined(__AVX2__)
    __m256i v = _mm256_loadu_si256((const __m256i*)base);
    __m256i t = _mm256_set1_epi8((char)h2);
    __m256i c = _mm256_cmpeq_epi8(v, t);
    return (uint32_t)_mm256_movemask_epi8(c);
#else
    __m128i v = _mm_loadu_si128((const __m128i*)base);
    __m128i t = _mm_set1_epi8((char)h2);
    __m128i c = _mm_cmpeq_epi8(v, t);
    return (uint32_t)_mm_movemask_epi8(c);
#endif
  }

  static inline uint32_t emptyMask(const uint8_t* base) {
#if defined(__AVX2__)
    __m256i v = _mm256_loadu_si256((const __m256i*)base);
    __m256i t = _mm256_set1_epi8((char)EMPTY);
    __m256i c = _mm256_cmpeq_epi8(v, t);
    return (uint32_t)_mm256_movemask_epi8(c);
#else
    __m128i v = _mm_loadu_si128((const __m128i*)base);
    __m128i t = _mm_set1_epi8((char)EMPTY);
    __m128i c = _mm_cmpeq_epi8(v, t);
    return (uint32_t)_mm_movemask_epi8(c);
#endif
  }

  template <class KArg, class VArg>
  void insertOrAssignImpl(KArg&& key, VArg&& value) {
    size_t h = hasher_(key);
    uint8_t h2 = h2_from_hash(h);
    size_t idx = h & mask_;

    for (;;) {
      uint8_t* base = ctrl_.data() + idx;

      // update existing
      uint32_t m = matchH2Mask(base, h2);
      while (m) {
        unsigned bit = ctz(m);
        size_t pos = idx + bit;
        if (ctrl_[pos] == h2 && eq_(slots_[pos].key, key)) {
          slots_[pos].value = std::forward<VArg>(value);
          return;
        }
        m &= (m - 1);
      }

      // insert into empty or deleted
      uint32_t e = emptyMask(base);
      uint32_t d = matchH2Mask(base, DELETED);
      uint32_t avail = e | d;

      if (avail) {
        unsigned bit = ctz(avail);
        size_t pos = idx + bit;

        if (ctrl_[pos] == DELETED) --deleted_;

        setCtrl(pos, h2);
        slots_[pos].key = std::forward<KArg>(key);
        slots_[pos].value = std::forward<VArg>(value);
        ++size_;
        return;
      }

      idx = (idx + GROUP_SIZE) & mask_;
    }
  }
};

} // namespace pxhash

#endif

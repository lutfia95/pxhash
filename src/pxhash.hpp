#ifndef PXHASH_HPP
#define PXHASH_HPP

#include <cstdint>
#include <cstdlib>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
  #include <immintrin.h>
#endif

namespace pxhash {

/*!\brief SwissTable-style control bytes (per-slot metadata).
 *
 * Control bytes encode EMPTY/DELETED sentinels or the 7-bit hash fingerprint.
 */
static constexpr uint8_t EMPTY   = 0x80;
static constexpr uint8_t DELETED = 0xFE;

#if defined(__AVX2__)
static constexpr size_t GROUP_SIZE = 32;
#else
static constexpr size_t GROUP_SIZE = 16;
#endif

/*!\brief Align n up to the next multiple of a.
 *
 * \param n The value to align.
 * \param a The alignment (must be a power of two).
 * \return The smallest multiple of \p a that is >= \p n.
 */
static inline size_t alignUp(size_t n, size_t a) { return (n + (a - 1)) & ~(a - 1); }

/*!\brief Compute the next power of two.
 *
 * \param n Input value.
 * \return Smallest power of two >= \p n (returns 1 for n <= 1).
 */
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

/*!\brief Extract a 7-bit fingerprint from the hash.
 *
 * The fingerprint is stored in control bytes for SIMD-accelerated filtering.
 */
static inline uint8_t h2_from_hash(size_t h) {
  return static_cast<uint8_t>((h >> (sizeof(size_t) * 8 - 7)) & 0x7F);
}

/*!\brief Simple key/value storage slot. */
template <class K, class V>
struct Slot {
  K key;
  V value;
};

template <typename KeyType, typename ValueType, typename Hash = std::hash<KeyType>,
          typename Eq = std::equal_to<KeyType>>
/*!\brief Compact open-addressing hash table inspired by SwissTable.
 *
 * Control bytes store EMPTY/DELETED or a 7-bit hash fingerprint.
 * Probing scans GROUP_SIZE control bytes at a time using SIMD.
 */
class PXHash {
public:
  explicit PXHash(size_t initial_capacity = 0) : hasher_(), eq_() {
    //std::cout << "[DEV] Reserving: " << initial_capacity << std::endl;
    if (initial_capacity) reserve(initial_capacity);
  }

  PXHash(const PXHash&) = delete;
  PXHash& operator=(const PXHash&) = delete;

  PXHash(PXHash&&) noexcept = default;
  PXHash& operator=(PXHash&&) noexcept = default;

  size_t size() const noexcept { return size_; }
  bool empty() const noexcept { return size_ == 0; }

  /*!\brief Reserve space for at least \p n elements. */
  void reserve(size_t n) {
    // Target max load ~ 7/8.
    size_t need = (n * 8) / 7 + 1;
    size_t cap = nextPowerOfTwo(need);
    if (cap < minCapacity()) cap = minCapacity();
    //std::cout << "[DEV] cap: " << cap << std::endl;
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

  /*!\brief Find a key and return its value via \p out_value.
   * \return True if the key is found, false otherwise.
   */
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

  /*!\brief Erase a key from the table.
   * \return True if the key was erased, false otherwise.
   */
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

  /*!\brief Control bytes for the hash table.
   *
   * The array has capacity_ + GROUP_SIZE bytes so the tail mirrors the first
   * GROUP_SIZE entries, allowing seamless SIMD loads at the end.
   */
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

  /*!\brief Set a control byte and keep the tail mirror in sync. */
  inline void setCtrl(size_t pos, uint8_t v) noexcept {
    ctrl_[pos] = v;
    if (pos < GROUP_SIZE) ctrl_[pos + capacity_] = v; // mirror
  }

  /*!\brief Initialize the table for a given capacity. */
  void initTable(size_t cap) {
    capacity_ = cap;
    mask_ = cap - 1;
    size_ = 0;
    deleted_ = 0;

    ctrl_.assign(capacity_ + GROUP_SIZE, EMPTY);
    slots_.resize(capacity_ + GROUP_SIZE);
    for (size_t i = 0; i < GROUP_SIZE; ++i) ctrl_[capacity_ + i] = ctrl_[i];
  }

  /*!\brief Rebuild the table to \p newCap capacity. */
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

  /*!\brief Grow or clean up the table before insert when load is high. */
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

  /*!\brief Return a bitmask of slots in the group matching \p h2. */
  static inline uint32_t matchH2Mask(const uint8_t* base, uint8_t h2) {
#if defined(__AVX2__)
    __m256i v = _mm256_loadu_si256((const __m256i*)base);
    __m256i t = _mm256_set1_epi8((char)h2);
    __m256i c = _mm256_cmpeq_epi8(v, t);
    return (uint32_t)_mm256_movemask_epi8(c);
#elif defined(__SSE2__)
    __m128i v = _mm_loadu_si128((const __m128i*)base);
    __m128i t = _mm_set1_epi8((char)h2);
    __m128i c = _mm_cmpeq_epi8(v, t);
    return (uint32_t)_mm_movemask_epi8(c);
#else
    uint32_t mask = 0;
    for (size_t i = 0; i < GROUP_SIZE; ++i) {
      if (base[i] == h2) mask |= (uint32_t{1} << i);
    }
    return mask;
#endif
  }

  /*!\brief Return a bitmask of EMPTY slots in the group. */
  static inline uint32_t emptyMask(const uint8_t* base) {
#if defined(__AVX2__)
    __m256i v = _mm256_loadu_si256((const __m256i*)base);
    __m256i t = _mm256_set1_epi8((char)EMPTY);
    __m256i c = _mm256_cmpeq_epi8(v, t);
    return (uint32_t)_mm256_movemask_epi8(c);
#elif defined(__SSE2__)
    __m128i v = _mm_loadu_si128((const __m128i*)base);
    __m128i t = _mm_set1_epi8((char)EMPTY);
    __m128i c = _mm_cmpeq_epi8(v, t);
    return (uint32_t)_mm_movemask_epi8(c);
#else
    uint32_t mask = 0;
    for (size_t i = 0; i < GROUP_SIZE; ++i) {
      if (base[i] == EMPTY) mask |= (uint32_t{1} << i);
    }
    return mask;
#endif
  }

  template <class KArg, class VArg>
  /*!\brief Insert or update in-place; assumes capacity is sufficient. */
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

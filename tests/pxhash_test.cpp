#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#include "pxhash.hpp"

namespace {

struct ConstantHash {
  std::size_t operator()(std::uint64_t) const noexcept { return 0; }
};

void test_insert_find_and_update() {
  pxhash::PXHash<std::string, int> map;

  assert(map.empty());

  map.insert("alpha", 1);
  map.insert("beta", 2);
  map.insert("alpha", 3);

  int value = 0;
  assert(map.size() == 2);
  assert(map.find("alpha", value));
  assert(value == 3);
  assert(map.find("beta", value));
  assert(value == 2);
  assert(!map.find("gamma", value));
}

void test_erase_and_reuse_deleted_slots() {
  pxhash::PXHash<int, int> map;

  for (int i = 0; i < 32; ++i) {
    map.insert(i, i * 10);
  }

  for (int i = 0; i < 16; ++i) {
    assert(map.erase(i));
  }

  assert(map.size() == 16);
  assert(!map.erase(999));

  for (int i = 100; i < 116; ++i) {
    map.insert(i, i * 10);
  }

  assert(map.size() == 32);

  int value = 0;
  for (int i = 0; i < 16; ++i) {
    assert(!map.find(i, value));
  }
  for (int i = 16; i < 32; ++i) {
    assert(map.find(i, value));
    assert(value == i * 10);
  }
  for (int i = 100; i < 116; ++i) {
    assert(map.find(i, value));
    assert(value == i * 10);
  }
}

void test_growth_preserves_values() {
  pxhash::PXHash<std::uint64_t, std::uint64_t> map;

  for (std::uint64_t i = 0; i < 512; ++i) {
    map.insert(i, i + 1000);
  }

  assert(map.size() == 512);

  std::uint64_t value = 0;
  for (std::uint64_t i = 0; i < 512; ++i) {
    assert(map.find(i, value));
    assert(value == i + 1000);
  }
}

void test_collision_heavy_workload() {
  pxhash::PXHash<std::uint64_t, std::uint64_t, ConstantHash> map;

  for (std::uint64_t i = 0; i < 96; ++i) {
    map.insert(i, i * 2);
  }

  std::uint64_t value = 0;
  for (std::uint64_t i = 0; i < 96; ++i) {
    assert(map.find(i, value));
    assert(value == i * 2);
  }

  for (std::uint64_t i = 0; i < 48; ++i) {
    assert(map.erase(i));
  }
  for (std::uint64_t i = 96; i < 144; ++i) {
    map.insert(i, i * 2);
  }

  assert(map.size() == 96);

  for (std::uint64_t i = 0; i < 48; ++i) {
    assert(!map.find(i, value));
  }
  for (std::uint64_t i = 48; i < 144; ++i) {
    assert(map.find(i, value));
    assert(value == i * 2);
  }
}

void test_move_insert_support() {
  pxhash::PXHash<std::string, std::string> map;
  std::string key = "k";
  std::string value = "payload";

  map.insert(std::move(key), std::move(value));

  std::string out;
  assert(map.find("k", out));
  assert(out == "payload");
}

}  // namespace

int main() {
  test_insert_find_and_update();
  test_erase_and_reuse_deleted_slots();
  test_growth_preserves_values();
  test_collision_heavy_workload();
  test_move_insert_support();
  return 0;
}

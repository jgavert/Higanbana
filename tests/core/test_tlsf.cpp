#include <catch2/catch.hpp>
#include <optional>
#include <vector>

struct Block {
  size_t offset;
  size_t size;
  operator bool() { return size != 0; }
};

class TLSFAllocator {
  struct TLSFSizeClass {
    size_t sizeClass;
    uint64_t slBitmap;
    std::vector<std::vector<Block>> freeBlocks;
  };

  struct TLSFControl {
    uint64_t flBitmap;
    std::vector<TLSFSizeClass> sizeclasses;
  };

  unsigned fli;        // first level index
  unsigned sli;        // second level index, typically 5
  unsigned sli_count;  // second level index, typically 5
  unsigned mbs;        // minimum block size
  TLSFControl control;
  size_t m_size;

  int fls(uint64_t size) {
    unsigned long index;
    return _BitScanReverse64(&index, size) ? index : -1;
  }

  int ffs(uint64_t size) {
    unsigned long index;
    return _BitScanForward64(&index, size) ? index : -1;
  }

  void mapping(size_t size, unsigned& fl, unsigned& sl) {
    fl = fls(size);
    sl = ((size ^ (1 << fl)) >> (fl - sli));
    // printf("%zu -> fl %u sl %u\n", size, fl, sl);
  }

  void initialize() {
    fli = fls(m_size);
    sli = 5;
    sli_count = 1 << sli;
    mbs = 16;
    control.flBitmap = 0;
    for (int i = 0; i <= fli; ++i) {
      size_t sizeClass = 1 << i;
      std::vector<std::vector<Block>> vectors;
      for (int k = 0; k < sli_count; ++k) {
        vectors.push_back(std::vector<Block>());
      }
      control.sizeclasses.push_back(TLSFSizeClass{sizeClass, 0, vectors});
    }
  }

  void remove_bit(uint64_t& value, int index) {
    value = value & (~(1 << index));
  }

  void set_bit(uint64_t& value, int index) { value |= (1 << index); }

  void insert(Block block, unsigned fl, unsigned sl) {
    auto& sizeClass = control.sizeclasses[fl];
    auto& secondLv = sizeClass.freeBlocks[sl];
    secondLv.push_back(block);
    set_bit(sizeClass.slBitmap, sl);
    set_bit(control.flBitmap, fl);
  }

  Block search_suitable_block(size_t size, unsigned fl, unsigned sl) {
    // first step, assume we got something at fl / sl location
    auto& secondLevel = control.sizeclasses[fl];
    auto& freeblocks = secondLevel.freeBlocks[sl];
    if (!freeblocks.empty() && freeblocks.back().size >= size) {
      auto block = freeblocks.back();
      freeblocks.pop_back();
      // remove bitmap bit
      if (freeblocks.empty()) remove_bit(secondLevel.slBitmap, sl);
      if (secondLevel.slBitmap == 0) remove_bit(control.flBitmap, fl);
      return block;
    } else {
      // second step, scan bitmaps for empty slots
      // create mask to ignore first bits, could be wrong
      auto mask = ~((1 << (fl + 1)) - 1);
      auto fl2 = ffs(control.flBitmap & mask);
      if (fl2 >= 0) {
        auto& secondLevel2 = control.sizeclasses[fl2];
        auto sl2 = ffs(secondLevel2.slBitmap);
        assert(!secondLevel2.freeBlocks[sl2].empty());
        if (sl2 >= 0 && secondLevel2.freeBlocks[sl2].back().size >= size) {
          auto block = secondLevel2.freeBlocks[sl2].back();
          secondLevel2.freeBlocks[sl2].pop_back();
          // remove bitmap bit
          if (secondLevel2.freeBlocks[sl2].empty())
            remove_bit(secondLevel2.slBitmap, sl2);
          if (secondLevel2.slBitmap == 0) remove_bit(control.flBitmap, fl2);
          return block;
        }
      }
    }

    return {};
  }

  Block split(Block& block, size_t size) {
    auto new_size = block.size - size;
    Block new_block = {block.offset + size, new_size};
    block.size = size;
    return new_block;
  }

  Block merge(Block block) {
    auto otf = block.offset;
    auto otf2 = block.offset + block.size;
    // oh no, nail in the coffin. BRUTEFORCE, we got not boundary tagging possible
    auto fl = 0;
    for (auto&& secondLevel : control.sizeclasses) {
      auto sl = 0;
      for (auto&& freeBlocks : secondLevel.freeBlocks) {
        auto iter = std::find_if(
            freeBlocks.begin(), freeBlocks.end(), [otf, otf2](Block b) {
              return (b.offset + b.size == otf) || (b.offset == otf2);
            });
        if (iter != freeBlocks.end()) {
          auto rb = *iter;
          freeBlocks.erase(iter);
          if (freeBlocks.empty()) remove_bit(secondLevel.slBitmap, sl);
          if (secondLevel.slBitmap == 0) remove_bit(control.flBitmap, fl);

          if (rb.offset + rb.size == otf) {
            rb.size += block.size;
            return rb;
          } else if (rb.offset == otf2) {
            block.size += rb.size;
            return block;
          }
        }
        sl++;
      }
      fl++;
    }
    return block;
  }

 public:
  TLSFAllocator(size_t size) : m_size(size) {
    initialize();
    unsigned fl, sl;
    mapping(size, fl, sl);
    insert({0, size}, fl, sl);
  }

  std::optional<Block> allocate(size_t size) {
    unsigned fl, sl, fl2, sl2;
    mapping(size, fl, sl);
    auto found_block = search_suitable_block(size, fl, sl);
    if (found_block) {
      if (found_block.size > size) {  // oh no, while loop
        auto remaining_block = split(found_block, size);
        mapping(remaining_block.size, fl2, sl2);
        insert(remaining_block, fl2, sl2);
      }
      return found_block;
    }
    return {};
  }

  void free(Block block) {
    // immediately merge with existing free blocks.
    unsigned fl, sl;
    auto size = block.size;
    auto big_free_block = merge(block);
    while (big_free_block.size != size)
    {
      size = big_free_block.size;
      big_free_block = merge(big_free_block);
    }
    mapping(big_free_block.size, fl, sl);
    insert(big_free_block, fl, sl);
  }
};

TEST_CASE("some basic allocation tests") {
  TLSFAllocator tlsf(4);
  auto block = tlsf.allocate(5);
  REQUIRE_FALSE(block);
  block = tlsf.allocate(2);
  REQUIRE(block);
  if (block) {
    REQUIRE(block.value().size == 2);
  }
  tlsf.free(block.value());
  block = tlsf.allocate(4);
  REQUIRE(block);
  if (block) {
    REQUIRE(block.value().size == 4);
  }

  tlsf.free(block.value());
  block = tlsf.allocate(3);
  REQUIRE(block);
  if (block) {
    REQUIRE(block.value().size == 3);
  }

  tlsf.free(block.value());
  block = tlsf.allocate(2);
  auto block2 = tlsf.allocate(2);
  REQUIRE(block);
  REQUIRE(block2);
  if (block && block2) {
    REQUIRE(block.value().size == 2);
    REQUIRE(block2.value().size == 2);
  }

  tlsf.free(block.value());
  tlsf.free(block2.value());
  block = tlsf.allocate(2);
  block2 = tlsf.allocate(1);
  REQUIRE(block);
  REQUIRE(block2);
  if (block && block2) {
    REQUIRE(block.value().size == 2);
    REQUIRE(block2.value().size == 1);
  }

  tlsf.free(block.value());
  tlsf.free(block2.value());
  auto block3 = tlsf.allocate(1);
  block = tlsf.allocate(2);
  block2 = tlsf.allocate(1);
  REQUIRE(block);
  REQUIRE(block2);
  REQUIRE(block3);
  if (block && block2 && block3) {
    REQUIRE(block.value().size == 2);
    REQUIRE(block2.value().size == 1);
    REQUIRE(block3.value().size == 1);
  }

  tlsf.free(block.value());
  tlsf.free(block2.value());
  tlsf.free(block3.value());
  auto block4 = tlsf.allocate(1);
  block = tlsf.allocate(1);
  block2 = tlsf.allocate(1);
  block3 = tlsf.allocate(1);
  REQUIRE(block);
  REQUIRE(block2);
  REQUIRE(block3);
  REQUIRE(block4);
  if (block && block2 && block3 && block4) {
    REQUIRE(block.value().size == 1);
    REQUIRE(block2.value().size == 1);
    REQUIRE(block3.value().size == 1);
    REQUIRE(block4.value().size == 1);
  }

  tlsf.free(block.value());
  tlsf.free(block2.value());
  tlsf.free(block3.value());
  tlsf.free(block4.value());
  block = tlsf.allocate(4);
  REQUIRE(block);
  if (block) {
    REQUIRE(block.value().size == 4);
  }
}

TEST_CASE("some basic allocation tests 2") {
  TLSFAllocator tlsf(70000);
  auto block = tlsf.allocate(50000);
  auto block2 = tlsf.allocate(20000);
  REQUIRE(block);
  REQUIRE(block2);
}
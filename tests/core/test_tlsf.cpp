#include <catch2/catch.hpp>

#include <higanbana/core/system/heap_allocator.hpp>

struct Block {
  uint64_t offset;
  uint64_t size;
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

  Block m_baseBlock;
  uint64_t fli;        // first level index
  uint64_t sli;        // second level index, typically 5
  unsigned sli_count;  // second level index, typically 5
  int mbs;        // minimum block size
  uint64_t min_fli;
  TLSFControl control;
  size_t m_usedSize;

  int fls(uint64_t size) noexcept {
    unsigned long index;
    return _BitScanReverse64(&index, size) ? index : -1;
  }

  int ffs(uint64_t size) noexcept {
    unsigned long index;
    return _BitScanForward64(&index, size) ? index : -1;
  }

  void mapping(size_t size, uint64_t& fl, uint64_t& sl) noexcept {
    fl = fls(size);
    sl = ((size ^ (1 << fl)) >> (fl - sli));
    fl = first_level_index(fl);
    // printf("%zu -> fl %u sl %u\n", size, fl, sl);
  }

  int first_level_index(int fli) noexcept {
    if (fli < min_fli )
      return 0;
    return fli - min_fli;
  }

  void initialize() noexcept {
    fli = fls(m_baseBlock.size);
    mbs = std::min(int(m_baseBlock.size), mbs);
    min_fli = fls(mbs);
    control.flBitmap = 0;
    if (0)
    {
      size_t sizeClass = 1;
      std::vector<std::vector<Block>> vectors;
      for (int k = 0; k < sli_count; ++k) {
        vectors.push_back(std::vector<Block>());
      }
      control.sizeclasses.push_back(TLSFSizeClass{sizeClass, 0, vectors});
    }
    for (int i = min_fli; i <= fli; ++i) {
      size_t sizeClass = 1 << i;
      std::vector<std::vector<Block>> vectors;
      for (int k = 0; k < sli_count; ++k) {
        vectors.push_back(std::vector<Block>());
      }
      control.sizeclasses.push_back(TLSFSizeClass{sizeClass, 0, vectors});
    }
  }

  void remove_bit(uint64_t& value, int index) noexcept { value = value ^ (1 << index); }

  void set_bit(uint64_t& value, int index) noexcept { value |= (1 << index); }

  void insert(Block block, unsigned fl, unsigned sl) noexcept {
    auto& sizeClass = control.sizeclasses[fl];
    auto& secondLv = sizeClass.freeBlocks[sl];
    secondLv.push_back(block);
    set_bit(sizeClass.slBitmap, sl);
    set_bit(control.flBitmap, fl);
  }

  Block search_suitable_block(size_t size, unsigned fl, unsigned sl) noexcept {
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
      auto mask = ~((1 << (fl+1)) - 1);
      auto fl2 = ffs(control.flBitmap & mask);
      if (fl2 >= 0) {
        auto& secondLevel2 = control.sizeclasses[fl2];
        assert(secondLevel2.sizeClass >= size && secondLevel2.slBitmap != 0);
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

  Block split(Block& block, size_t size) noexcept {
    auto new_size = block.size - size;
    Block new_block = {block.offset + size, new_size};
    block.size = size;
    return new_block;
  }

  Block merge(Block block) noexcept {
    auto otf = block.offset;
    auto otf2 = block.offset + block.size;
    // oh no, nail in the coffin. BRUTEFORCE, we got not boundary tagging
    // possible sped up by using bitmaps to avoid checking empty vectors
    auto fl = 0;

    // scan through only the memory where blocks reside using bitfields
    auto flBM = control.flBitmap;
    while (flBM != 0) {
      auto fl = ffs(flBM);
      remove_bit(flBM, fl);

      auto& secondLevel = control.sizeclasses[fl];
      // use the bitmap to only check relevant vectors
      auto slBM = secondLevel.slBitmap;
      while (slBM != 0) {
        auto sl = ffs(slBM);
        remove_bit(slBM, sl);

        auto& freeBlocks = secondLevel.freeBlocks[sl];

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
      }
    }
    return block;
  }

 public:
  TLSFAllocator(Block initialBlock, size_t minimumBlockSize = 16, int sli = 3)
  : m_baseBlock(initialBlock), mbs(minimumBlockSize), sli(sli), sli_count(1 << sli) {
    initialize();
    uint64_t fl, sl;
    mapping(m_baseBlock.size, fl, sl);
    insert(initialBlock, fl, sl);
  }

  TLSFAllocator(size_t size, size_t minimumBlockSize = 16, int sli = 3)
  : m_baseBlock({0,size}), mbs(minimumBlockSize), sli(sli), sli_count(1 << sli) {
    initialize();
    uint64_t fl, sl;
    mapping(m_baseBlock.size, fl, sl);
    insert(m_baseBlock, fl, sl);
  }

  std::optional<Block> allocate(size_t size, size_t alignment = 1) noexcept {
    size = std::max(size, size_t(mbs));
    uint64_t fl, sl, fl2, sl2;
    mapping(size, fl, sl);
    auto found_block = search_suitable_block(size, fl, sl);
    if (found_block) {
      auto overAlign = found_block.offset % alignment;
      if (overAlign != 0 && found_block.size > size + alignment) {
        // fix alignment
        auto overAlignmentFix = alignment - overAlign;
        auto smallFixBlock = Block{found_block.offset, overAlign};
        found_block.offset += overAlignmentFix;
        assert(found_block.offset % alignment == 0);
        // guaranteed that nothing will merge with this block.
        mapping(smallFixBlock.size, fl2, sl2);
        insert(smallFixBlock, fl2, sl2);
      }
      if (found_block.size > size) {
        auto remaining_block = split(found_block, size);
        mapping(remaining_block.size, fl2, sl2);
        insert(remaining_block, fl2, sl2);
      }
      m_usedSize += found_block.size;
      return found_block;
    }
    return {};
  }

  void free(Block block) noexcept {
    // immediately merge with existing free blocks.
    m_usedSize -= block.size;
    uint64_t fl, sl;
    auto size = block.size;
    auto big_free_block = merge(block);
    while (big_free_block.size != size)  // while loop here
    {
      size = big_free_block.size;
      big_free_block = merge(big_free_block);
    }
    mapping(big_free_block.size, fl, sl);
    insert(big_free_block, fl, sl);
  }

  size_t size() const noexcept {
    return m_baseBlock.size - m_usedSize;
  }
  size_t max_size() const noexcept {
    return m_baseBlock.size;
  }
  size_t size_allocated() const noexcept {
    return m_usedSize;
  }
};

TEST_CASE("some basic allocation tests") {
  higanbana::HeapAllocator tlsf(4, 1);
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
  higanbana::HeapAllocator tlsf(70000);
  auto block = tlsf.allocate(50000);
  auto block2 = tlsf.allocate(20000);
  REQUIRE(block);
  REQUIRE(block2);
}

TEST_CASE("stranger tests") {
  auto count = 100;
  auto sum = (count * (count + 1)) / 2;
  higanbana::HeapAllocator tlsf(sum, 1);
  higanbana::vector<higanbana::RangeBlock> blocks;
  for (int i = 1; i <= count; ++i) {
    auto block = tlsf.allocate(i);
    REQUIRE(block);
    blocks.push_back(block.value());
  }
  auto block = tlsf.allocate(1);
  REQUIRE_FALSE(block);
  tlsf.free(blocks.back());
  block = tlsf.allocate(1);
  REQUIRE(block);
}

TEST_CASE("alignment tests") {
  higanbana::HeapAllocator tlsf(50, 1);
  auto block = tlsf.allocate(3, 1);
  auto block2 = tlsf.allocate(3*3*3, 9);
  REQUIRE(block);
  REQUIRE(block.value().offset % 1 == 0);
  REQUIRE(block.value().size == 3);
  REQUIRE(block2);
  REQUIRE(block2.value().offset % 9 == 0);
  REQUIRE(block2.value().size == 3*3*3);
  auto block4 = tlsf.allocate(10, 2);
  REQUIRE(block4);
  REQUIRE(block4.value().size == 10);
  auto block3 = tlsf.allocate(3, 1);
  REQUIRE(block3);
  REQUIRE(block3.value().size == 3);
  auto block5 = tlsf.allocate(10, 1);
  REQUIRE(block5);
  REQUIRE(block5.value().size == 10);

  tlsf.free(block2.value());
  block2 = tlsf.allocate(3*3, 1);
  
  REQUIRE(block2);
  REQUIRE(block2.value().offset % 1 == 0);
  REQUIRE(block2.value().size == 3*3);
}

TEST_CASE("alignment tests with minimum block size") {
  higanbana::HeapAllocator tlsf(100, 16);
  auto block = tlsf.allocate(3, 1);
  auto block2 = tlsf.allocate(3*3*3, 9);
  REQUIRE(block);
  REQUIRE(block.value().offset % 1 == 0);
  REQUIRE(block.value().size == 16);
  REQUIRE(block2);
  REQUIRE(block2.value().offset % 9 == 0);
  REQUIRE(block2.value().size == 3*3*3);
  auto block4 = tlsf.allocate(10, 2);
  REQUIRE(block4);
  REQUIRE(block4.value().size == 16);
  auto block3 = tlsf.allocate(3, 1);
  REQUIRE(block3);
  REQUIRE(block3.value().size == 16);
}
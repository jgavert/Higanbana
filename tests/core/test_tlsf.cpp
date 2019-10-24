#include <catch2/catch.hpp>

#include <vector>
#include <optional>


struct Block
{
  size_t offset;
  size_t size;
  operator bool()
  {
    return size != 0;
  }
};

class TLSFAllocator
{
  struct TLSFSizeClass
  {
    size_t sizeClass;
    uint64_t slBitmap;
    std::vector<std::vector<Block>> freeBlocks;
  };

  struct TLSFControl
  {
    uint64_t flBitmap;
    std::vector<TLSFSizeClass> sizeclasses;
  };

  unsigned fli; // first level index
  unsigned sli; // second level index, typically 5
  unsigned sli_count; // second level index, typically 5
  unsigned mbs; // minimum block size
  TLSFControl control;
  size_t m_size;

  int fls(uint64_t size)
  {
    unsigned long index;
    return _BitScanReverse64(&index, size) ? index : -1;
  }

  int ffs(uint64_t size)
  {
    unsigned long index;
    return _BitScanForward64(&index, size) ? index : -1;
  }

  void mapping(size_t size, unsigned& fl, unsigned& sl)
  {
    fl = fls(size);
    sl = ((size ^ (1<<fl)) >> (fl - sli));
    //printf("%zu -> fl %u sl %u\n", size, fl, sl);
  }

  void initialize()
  {
    fli = ffs(m_size);
    sli = 5;
    sli_count = 1 << sli;
    mbs = 16;
    control.flBitmap = 0;
    for (int i = 0; i <= fli; ++i)
    {
      size_t sizeClass = 1 << i;
      std::vector<std::vector<Block>> vectors;
      for (int k = 0; k < sli; ++k)
      {
        vectors.push_back(std::vector<Block>());
      }
      control.sizeclasses.push_back(TLSFSizeClass{sizeClass, 0, vectors});
    }
  }

  void remove_bit(uint64_t& value, int index)
  {
    value = value & (~(1 << index));
  }

  void set_bit(uint64_t& value, int index)
  {
    value |= (1 << index);
  }

  void insert(Block block, unsigned fl, unsigned sl)
  {
    auto& sizeClass = control.sizeclasses[fl];
    auto& secondLv = sizeClass.freeBlocks[sl];
    secondLv.push_back(block);
    set_bit(sizeClass.slBitmap, sl);
    set_bit(control.flBitmap, fl);
  }

  Block search_suitable_block(size_t size, unsigned fl, unsigned sl)
  {
    // first step, assume we got something at fl / sl location
    auto& sizeClass = control.sizeclasses[fl];
    auto& secondLv = sizeClass.freeBlocks[sl];
    if (!secondLv.empty() && sizeClass.sizeClass >= size) {
      auto block = secondLv.back();
      secondLv.pop_back();
      // remove bitmap bit
      if (secondLv.empty()) remove_bit(sizeClass.slBitmap, sl);
      if (sizeClass.slBitmap == 0) remove_bit(control.flBitmap, fl);
      return block;
    }
    else {
      // second step, scan bitmaps for empty slots
      auto mask = ~((1 << (fl+1))-1); // create mask to ignore first bits, could be wrong
      auto fl2 = ffs(control.flBitmap & mask);
      if (fl2 >= 0) {
        auto& sizeClass2 = control.sizeclasses[fl2];
        auto sl2 = ffs(sizeClass2.slBitmap);
        if (sl2 >= 0 && sizeClass2.sizeClass >= size) {
          assert(!sizeClass2.freeBlocks[sl2].empty());
          auto block = sizeClass2.freeBlocks[sl2].back();
          sizeClass2.freeBlocks[sl2].pop_back();
          // remove bitmap bit
          if (secondLv.empty()) remove_bit(sizeClass2.slBitmap, sl2);
          if (sizeClass2.slBitmap == 0) remove_bit(control.flBitmap, fl2);
          return block;
        }
      }
    }
    
    return {};
  }

  Block split(Block& block)
  {
    auto new_size = block.size / 2;
    Block new_block = {block.offset+new_size, new_size};
    block.size = new_size;
    return new_block;
  }

  Block merge(Block block)
  {
    // try to merge with some existing block???????????????
    auto otf = block.offset;
    auto otf2 = block.offset + block.size;
    // BRUTEFORCE
    auto fl = 0;
    for (auto&& firstLevel : control.sizeclasses)
    {
      auto sl = 0;
      for (auto&& secondLevel : firstLevel.freeBlocks)
      {
        auto iter = std::find_if(secondLevel.begin(), secondLevel.end(), [otf, otf2](Block b){
          return (b.offset+b.size == otf) || (b.offset == otf2);
        });
        if (iter != secondLevel.end())
        {
          auto rb = *iter;
          secondLevel.erase(iter);
          if (secondLevel.empty()) remove_bit(firstLevel.slBitmap, sl);
          if (firstLevel.slBitmap == 0) remove_bit(control.flBitmap, fl);

          if (rb.offset+rb.size == otf) {
            rb.size += block.size;
            return rb;
          }
          else if(rb.offset == otf2) {
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
  TLSFAllocator(size_t size)
    : m_size(size)
  {
    initialize();
    unsigned fl, sl;
    mapping(size, fl, sl);
    insert({0, size}, fl, sl);
  }

  std::optional<Block> allocate(size_t size)
  {
    unsigned fl, sl, fl2, sl2;
    mapping(size, fl, sl);
    auto found_block = search_suitable_block(size,fl,sl);
    if (found_block) {
      while (found_block.size >= size*2)
      {
        auto remaining_block = split(found_block);
        mapping(remaining_block.size, fl2, sl2);
        insert(remaining_block, fl2, sl2); // O(1)
      }
      return found_block;
    }
    return {};
  }

  void free(Block block)
  {
    // immediately merge with existing free blocks.
    unsigned fl, sl;
    auto big_free_block = merge(block); // O(1)
    mapping(big_free_block.size, fl, sl);
    insert(big_free_block, fl, sl); // O(1)
  }
};

TEST_CASE("some basic allocation tests")
{
  TLSFAllocator tlsf(4);
  auto block = tlsf.allocate(5);
  REQUIRE_FALSE(block);
  block = tlsf.allocate(2);
  REQUIRE(block);
  if (block)
  {
    REQUIRE(block.value().size == 2);
  }
  tlsf.free(block.value());
  block = tlsf.allocate(4);
  REQUIRE(block);
  if (block)
  {
    REQUIRE(block.value().size == 4);
  }

  tlsf.free(block.value());
  block = tlsf.allocate(3);
  REQUIRE(block);
  if (block)
  {
    REQUIRE(block.value().size == 4);
  }

  tlsf.free(block.value());
  block = tlsf.allocate(2);
  auto block2 = tlsf.allocate(2);
  REQUIRE(block);
  REQUIRE(block2);
  if (block && block2)
  {
    REQUIRE(block.value().size == 2);
    REQUIRE(block2.value().size == 2);
  }
}
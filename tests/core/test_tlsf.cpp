#include <catch2/catch.hpp>

#include <vector>
#include <optional>


struct Block
{
  size_t offset;
  size_t size;
};

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

class TLSFAllocator
{
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
  }

  void initialize()
  {
    fli = ffs(m_size);
    sli = 5;
    sli_count = 1 << sli;
    mbs = 16;
    control.flBitmap = 0;
    for (int i = 0; i < fli; ++i)
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
public:

  TLSFAllocator(size_t size)
    : m_size(size)
  {
    initialize();
  }

  std::optional<Block> allocate(size_t size)
  {
    unsigned fl, sl;
    mapping(size, fl, sl);
    printf("%zu -> fl %u sl %u\n", size, fl, sl);
    return {};
  }

  void free(Block block)
  {
    // immediately merge with existing free blocks.
  }
};

TEST_CASE("can allocate")
{
  TLSFAllocator tlsf(4);
  auto block = tlsf.allocate(5);
  REQUIRE(!block);
  block = tlsf.allocate(5);
  REQUIRE(block);
  if (block)
  {
    REQUIRE(block.value().size == 1);
  }
}
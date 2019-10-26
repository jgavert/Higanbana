#include "higanbana/core/system/heap_allocator.hpp"

namespace higanbana
{
HeapAllocator::HeapAllocator()
: m_baseBlock({0,0}), mbs(1), sli(1), sli_count(1 << sli) {
}
HeapAllocator::HeapAllocator(RangeBlock initialBlock, size_t minimumBlockSize, int sli)
: m_baseBlock(initialBlock), mbs(minimumBlockSize), sli(sli), sli_count(1 << sli), m_usedSize(0) {
  initialize();
  uint64_t fl, sl;
  mapping(m_baseBlock.size, fl, sl);
  insert(initialBlock, fl, sl);
}

HeapAllocator::HeapAllocator(size_t size, size_t minimumBlockSize, int sli)
: m_baseBlock({0, size}), mbs(minimumBlockSize), sli(sli), sli_count(1 << sli), m_usedSize(0) {
  initialize();
  uint64_t fl, sl;
  mapping(m_baseBlock.size, fl, sl);
  insert(m_baseBlock, fl, sl);
}

std::optional<RangeBlock> HeapAllocator::allocate(size_t size, size_t alignment) noexcept {
  auto origSize = size;
  size = std::max(size, size_t(mbs));
  uint64_t fl, sl, fl2, sl2;
  mapping(size, fl, sl);
  auto found_block = search_suitable_block(size, fl, sl);

  if (found_block) {
    auto overAlign = found_block.offset % alignment;

    if (overAlign != 0 && found_block.size < size + alignment) {
      // we got a block that was too small, need to find a new one
      insert(found_block, fl, sl);
      size += alignment;
      mapping(size, fl, sl);
      found_block = search_suitable_block(size, fl, sl);
      if (!found_block)
        return {};
      overAlign = found_block.offset % alignment;
    }

    if (overAlign != 0 && found_block.size >= size) {
      // fix alignment
      auto overAlignmentFix = alignment - overAlign;
      auto smallFixBlock = RangeBlock{found_block.offset, overAlignmentFix};
      found_block.offset += smallFixBlock.size;
      found_block.size -= smallFixBlock.size;
      HIGAN_ASSERT(found_block.offset % alignment == 0, "Alignment failure...");
      // guaranteed that nothing will merge with this block.
      mapping(smallFixBlock.size, fl2, sl2);
      insert(smallFixBlock, fl2, sl2);
    }
    if (found_block.size > size) {
      auto remaining_block = split(found_block, size);
      mapping(remaining_block.size, fl2, sl2);
      insert(remaining_block, fl2, sl2);
    }
    HIGAN_ASSERT(found_block.offset % alignment == 0, "Alignment failure...");
    HIGAN_ASSERT(found_block.size >= origSize, "Allocation failure...");
    m_usedSize += found_block.size;
    return found_block;
  }
  return {};
}

void HeapAllocator::free(RangeBlock block) noexcept {
  // immediately merge with existing free blocks.
  //HIGAN_ASSERT(block.size != 0, "Invalid free.");

  // check if this memory doesn't exist yet, error out if it does.
#if 0
  for (auto&& sizeClass : control.sizeclasses)
  {
    auto cb = block.offset, ce = block.offset+block.size;
    for (auto&& freeBlocks : sizeClass.freeBlocks)
    {
      for (auto&& fb : freeBlocks)
      {
        auto b = fb.offset, e = fb.offset+fb.size;
        HIGAN_ASSERT(cb != b, "No same beginnings");
        if (cb > b)
          HIGAN_ASSERT(cb >= e, "block must be to left");
        if (ce > e)
          HIGAN_ASSERT(ce >= b, "block must be to left");
      }
    }
  }
#endif

  if (block.size == 0)
    return;
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
}
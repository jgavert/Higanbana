#pragma once
#include <vector>
#include <cmath>
#include <memory>
#include <cassert>
#include <cstdlib>
#include <algorithm>


namespace higanbana
{

  class HistoPyramid
  {
  private:
    std::shared_ptr<std::vector<size_t>> m_vector;
    size_t m_level;
    size_t m_row;

    size_t          countMaxSize(size_t a);
    inline uint32_t EncodeMorton2D(uint32_t x, uint32_t y)
    {
      return (Part1By1(y) << 1) + Part1By1(x);
    }

    inline uint32_t Part1By1(uint32_t x)
    {
      x &= 0x0000ffff;                  // x = ---- ---- ---- ---- fedc ba98 7654 3210
      x = (x ^ (x << 8)) & 0x00ff00ff; // x = ---- ---- fedc ba98 ---- ---- 7654 3210
      x = (x ^ (x << 4)) & 0x0f0f0f0f; // x = ---- fedc ---- ba98 ---- 7654 ---- 3210
      x = (x ^ (x << 2)) & 0x33333333; // x = --fe --dc --ba --98 --76 --54 --32 --10
      x = (x ^ (x << 1)) & 0x55555555; // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
      return x;
    }
  public:
    HistoPyramid(size_t amountOfData);
    // inlined public
    inline size_t getSize()
    {
      return m_vector->size();
    }

    inline size_t getlevels()
    {
      return m_level;
    }

    inline size_t getTreeTopValue()
    {
      return (*m_vector)[m_vector->size() - 1];
    }

    inline void insert(uint32_t x, uint32_t y, size_t val)
    {
      assert(x < m_row && y < m_row);
      size_t mortonIndex = static_cast<size_t>(EncodeMorton2D(x, y));
      (*m_vector)[mortonIndex] = val;
    }

    // public
    void          buildPyramid();
    size_t        levelOffset(int level);
    void          print(int level);
    void          printPyramid();
    std::vector<std::pair<size_t, size_t>> indexValueExtractor();
  };
}

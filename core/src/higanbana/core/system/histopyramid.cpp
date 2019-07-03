#include "higanbana/core/system/histopyramid.hpp"

using namespace higanbana;

HistoPyramid::HistoPyramid(size_t amountOfData):m_level(0)
{
  m_vector = std::make_shared<std::vector<size_t>>(countMaxSize(amountOfData), 0);
  m_row = static_cast<size_t>(std::sqrt(std::pow(4, m_level)));
}

size_t HistoPyramid::countMaxSize(size_t a)
{
  size_t s = 1;
  size_t level = 0;
  size_t levelsize = 1;
  while (a > levelsize)
  { 
    levelsize = static_cast<size_t>(std::pow(4, ++level));
    s += levelsize;
  }
  m_level = level;
  return s;
}

void HistoPyramid::buildPyramid()
{
  size_t lowerLevel = 0;
  size_t higherLevel = 0;
  size_t higherLevelSize = 0;
  for (int l = static_cast<int>(m_level)-1; l >= 0; --l)
  {
    lowerLevel = higherLevel;
    higherLevel = levelOffset(l);
    higherLevelSize = static_cast<size_t>(std::pow(4, l));
    for (size_t i = 0; i < higherLevelSize; ++i)
    {
      size_t workIndex = higherLevel + i;
      size_t pi = lowerLevel + i * 4;
      (*m_vector)[workIndex] = (*m_vector)[pi] + (*m_vector)[pi+1] + (*m_vector)[pi+2] + (*m_vector)[pi+3];
    }
  }
}

size_t HistoPyramid::levelOffset(int level)
{
  int current_level = static_cast<int>(m_level);
  size_t offset = 0;
  while (current_level != level)
  {
    offset += static_cast<int>(std::pow(4, current_level));
    --current_level;
  }
  return offset;
}

void HistoPyramid::print(int level)
{
  if (level > static_cast<int>(m_level))
    level = static_cast<int>(m_level);
  if (level < 0)
    level = 0;
  int levelRow = static_cast<int>(std::sqrt(std::pow(4, level)));
  // last level first
  size_t calculatedMortonOffset = levelOffset(level);
  /*
  for (int i = 0; i < levelRow; ++i)
    printf("------");
  printf("-\n");
  */
  for (int i = 0; i < levelRow; ++i)
  {
    for (int k = 0; k < levelRow; ++k)
    {
      printf(" %3zu ", (*m_vector)[calculatedMortonOffset+EncodeMorton2D(i, k)]);
    }
    /*
    printf("|\n");
    for (int i = 0; i < levelRow; ++i)
      printf("------");
      */
    printf("\n");
  }
}

void HistoPyramid::printPyramid()
{
  for (int i = static_cast<int>(getlevels()); i >= 0; --i)
  {
    print(i);
  }
}

std::vector<std::pair<size_t, size_t>> HistoPyramid::indexValueExtractor()
{
  size_t arraySize = getTreeTopValue();
  if (arraySize == 0)
    return std::vector<std::pair<size_t, size_t>>();
  std::vector<std::pair<size_t, size_t>> ret(arraySize, {0,0});

  if (m_level == 1)
  {
    ret.push_back({ 0, arraySize });
    return ret;
  }

  auto& vec = *m_vector;
  for (size_t i = 0; i < arraySize; ++i)
  {
    size_t offset = 0; // current levels offset
    size_t currentLevelOffset = levelOffset(1);
    size_t foundMortonIndex = currentLevelOffset;
    size_t rangeOffset = 0;
    size_t chosen = offset;
    size_t roa[5] = { 0 };
    for (size_t l = 1; l <= m_level; ++l)
    {
      chosen = offset;
      foundMortonIndex = currentLevelOffset + offset;
      // branchless?
      // find range where you are in
      roa[0] = rangeOffset;
      roa[1] = roa[0] + vec[foundMortonIndex];
      roa[2] = roa[1] + vec[foundMortonIndex+1];
      roa[3] = roa[2] + vec[foundMortonIndex+2];
      roa[4] = roa[3] + vec[foundMortonIndex+3];
      int c = 3;
      c = std::max(static_cast<int>((roa[3] <= i)) * c, 2);
      c = std::max(static_cast<int>((roa[2] <= i)) * c, 1);
      c = std::max(static_cast<int>((roa[1] <= i)) * c, 0);
      rangeOffset = roa[c];
      chosen += c;
      // update offset for next level 2x2 base index
      offset = chosen * 4;
      currentLevelOffset -= static_cast<size_t>(std::pow(4, l+1));
      // change the foundMortonIndex closer to the real index
    }
    foundMortonIndex = chosen;
    // reach base texture and extract the value
    size_t insideEle = i-rangeOffset;
    ret[i] = { foundMortonIndex, insideEle };
    //i += vec[foundMortonIndex] - 1;
  }
  return ret;
}

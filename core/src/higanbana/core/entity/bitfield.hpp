#pragma once
#include <array>
#include <vector>
#include <algorithm>
#include <emmintrin.h>
#include <smmintrin.h>
#include <tmmintrin.h>
#include <nmmintrin.h>

namespace higanbana
{
  inline size_t calcCtzll(uint64_t v) {
#if 1
    unsigned long ret;
    _BitScanForward64(&ret, v);
    return static_cast<size_t>(ret);
#else
    return __builtin_ctzll(a);
#endif
  }
  namespace bitOps
  {
    bool checkbit(const __m128i& vec, int n);
    void setbit(__m128i& vec, int n);
    void clearbit(__m128i& vec, int n);
    void togglebit(__m128i& vec, int n);
    size_t popcount(const __m128i& v);
  }

  namespace bitfieldUtils
  {
    void setZero(__m128i* data, int count);
    void setFull(__m128i* data, int count);

    template <size_t rsize>
    std::array<__m128i, rsize> init()
    {
      std::array<__m128i, rsize> result;
      __m128i* p = reinterpret_cast<__m128i*>(result.data());
      for (size_t i = 0; i < rsize; ++i)
      {
        _mm_store_si128(p + i, _mm_setzero_si128());
      }
      return result;
    }

    template <size_t rsize>
    std::array<__m128i, rsize> init_full()
    {
      std::array<__m128i, rsize> result;
      __m128i* p = reinterpret_cast<__m128i*>(result.data());
      for (size_t i = 0; i < rsize; ++i)
      {
        _mm_store_si128(p + i, _mm_sub_epi64(_mm_set_epi64x(0LL, 0LL), _mm_set_epi64x(1LL, 1LL)));
      }
      return std::move(result);
    }

    template <size_t rsize>
    size_t countOneBlocks(const std::array<__m128i, rsize>& vec)
    {
      size_t res = 0;
      const long long vl = (std::numeric_limits<long long>::max)();
      const __m128i mask = { vl, vl };
      for (size_t i = 0; i < rsize; ++i)
      {
        res += !_mm_test_all_zeros(vec[i], mask);
      }
      return res;
    }

    template <size_t rsize>
    size_t countPopulatedBlocks(const std::array<__m128i, rsize>& vec)
    {
      size_t res = 0;
      for (size_t i = 0; i < rsize; ++i)
      {
        res += (bitOps::popcount(vec[i]) > 0);
      }
      return res;
    }
  }

  template <size_t rsize>
  class Bitfield
  {
  private:
    std::array<__m128i, rsize> m_table;

  public:
    Bitfield() :m_table(bitfieldUtils::init<rsize>()) {}
    Bitfield(bool) :m_table(bitfieldUtils::init_full<rsize>()) {}
    Bitfield(std::array<__m128i, rsize> data) :m_table(data) {}

    inline size_t size() const
    {
      return rsize;
    }

    inline void setIdxBit(size_t n)
    {
      bitOps::setbit(m_table[n / 128], n % 128);
    }

    inline bool checkIdxBit(size_t n)
    {
      return bitOps::checkbit(m_table[n / 128], n % 128);
    }

    inline void toggleIdxBit(size_t n)
    {
      return bitOps::togglebit(m_table[n / 128], n % 128);
    }
    inline void clearIdxBit(size_t n)
    {
      return bitOps::clearbit(m_table[n / 128], n % 128);
    }

    inline size_t nextPopBucket(size_t index) const
    {
      while (index < rsize && bitOps::popcount(m_table[index]) == 0)
      {
        ++index;
      }
      return index;
    }

    inline int64_t nextBucketWithRoom(size_t index) const
    {
      while (index < rsize && bitOps::popcount(m_table[index]) == 128)
      {
        ++index;
      }
      if (index >= rsize)
        return -1;
      return index;
    }

    inline int64_t skip_find_firstEmpty_full() const
    {
      int64_t partiallyEmpty = nextBucketWithRoom(0);
      if (partiallyEmpty == -1)
        return -1;
      return skip_find_firstEmpty(partiallyEmpty);
    }

    inline int64_t skip_find_firstEmpty_full_offset(size_t index_offset) const
    {
      size_t block = index_offset / 128;
      size_t block_offset = index_offset % 128;
      int64_t partiallyEmpty = nextBucketWithRoom(block);
      if (partiallyEmpty == -1)
        return -1;
      return skip_find_firstEmpty_offset(partiallyEmpty, block_offset);
    }

    inline int64_t skip_find_firstEmpty(size_t block) const
    {
      uint64_t a = ~(_mm_extract_epi64(m_table[block], 0));
      uint64_t b = ~(_mm_extract_epi64(m_table[block], 1));
      size_t offset = block * 128;
      size_t inner_idx = calcCtzll(a);
      if (a)
      {
        return inner_idx + offset;
      }
      else if (b)
      {
        inner_idx = calcCtzll(b);
        return inner_idx + 64 + offset;
      }
      return -1;
    }

    inline int64_t skip_find_firstEmpty_offset(size_t block, size_t offset) const
    {
      uint64_t a = ~(_mm_extract_epi64(m_table[block], 0));
      uint64_t b = ~(_mm_extract_epi64(m_table[block], 1));
      size_t offset_tmp = 0;
      size_t blockOffset = block * 128;
      size_t inner_idx = 0;

      if (offset >= 64) // only b to check
      {
        offset_tmp = std::min(64, static_cast<int>(offset) - 64);
        for (size_t i = 0; i < offset_tmp; ++i)
        {
          b ^= (-0LL ^ b) & (1LL << i);
        }
        if (b)
        {
          inner_idx = calcCtzll(b);
          return inner_idx + 64 + blockOffset;
        }
        return -1;
      }
      // both a and b to check
      offset_tmp = offset;
      for (size_t i = 0; i < offset_tmp; ++i)
      {
        a ^= (-0LL ^ a) & (1LL << i);
      }

      if (a)
      {
        inner_idx = calcCtzll(a);
        return inner_idx + blockOffset;
      }
      else if (b)
      {
        inner_idx = calcCtzll(b);
        return inner_idx + 64 + blockOffset;
      }
      return -1;
    }

    template<size_t TableSize>
    inline size_t skip_find_indexes(std::array<size_t, TableSize>& table, size_t table_size, size_t index) const
    {
      uint64_t a = _mm_extract_epi64(m_table[index], 0);
      uint64_t b = _mm_extract_epi64(m_table[index], 1);
      size_t offset = index * 128;
      size_t idx = table_size;
      size_t inner_idx = calcCtzll(a);
      while (a)
      {
        table[idx++] = inner_idx + offset;
        a &= ~(1LL << (inner_idx));
        inner_idx = calcCtzll(a);
      }
      inner_idx = calcCtzll(b);
      while (b)
      {
        table[idx++] = inner_idx + 64 + offset;
        b &= ~(1LL << (inner_idx));
        inner_idx = calcCtzll(b);
      }
      return idx;
    }

    template<size_t TableSize>
    size_t returnIndexes(std::array<size_t, TableSize>& table, size_t table_size, size_t index) const
    {
      uint64_t a = _mm_extract_epi64(m_table[index], 0);
      uint64_t b = _mm_extract_epi64(m_table[index], 1);
      size_t offset = index * 128;
      size_t idx = table_size;
      if (a > 0)
      {
        for (size_t i = offset; i < offset + 64; ++i)
        {
          if (a & (1LL << i))
          {
            table[idx] = i;
            ++idx;
          }
        }
      }
      if (b > 0)
      {
        for (size_t i = offset + 64; i < offset + 128; ++i)
        {
          if (b & (1LL << i))
          {
            table[idx] = i;
            ++idx;
          }
        }
      }
      return idx;
    }

    template<size_t TableSize, typename Func>
    inline size_t skip_find_indexes_prefetch(std::array<size_t, TableSize>& table, size_t table_size, size_t index, Func&& pre) const
    {
      uint64_t a = _mm_extract_epi64(m_table[index], 0);
      uint64_t b = _mm_extract_epi64(m_table[index], 1);
      size_t offset = index * 128;
      size_t idx = table_size;
      size_t inner_idx = calcCtzll(a);
      while (a)
      {
        size_t value = inner_idx + offset;
        table[idx] = value;
        ++idx;
        pre(value);
        a &= ~(1LL << (inner_idx));
        inner_idx = calcCtzll(a);
      }
      inner_idx = calcCtzll(b);
      while (b)
      {
        size_t value = inner_idx + offset + 64;
        table[idx] = value;
        ++idx;
        pre(value);
        b &= ~(1LL << (inner_idx));
        inner_idx = calcCtzll(b);
      }
      return idx;
    }

    std::array<__m128i, rsize>&& mv_data()
    {
      return std::move(m_table);
    }

    std::array<__m128i, rsize>& data()
    {
      return m_table;
    }

    std::array<__m128i, rsize> cp_data() const
    {
      return m_table;
    }
    size_t countElements() const
    {
      size_t res = 0;
      for (size_t i = 0; i < rsize; ++i)
      {
        res += bitOps::popcount(m_table[i]);
      }
      return res;
    }

    inline size_t contiguous_full_buckets(size_t index) const
    {
      size_t count = 0;
      while (index < rsize && bitOps::popcount(m_table[index]) == 128)
      {
        ++count;
        ++index;
      }
      return count;
    }

    inline size_t popcount_element(size_t index) const
    {
      return bitOps::popcount(m_table[index]);
    }
  };
/*
    // this is bullshit, Containts the structure but needs rewriting.
    // __builtin_ctzll counts the leading zeros, think about this
    size_t countClearedBitsInBucket(size_t startIndex)
    {
      size_t table = startIndex / 128;
      size_t offset = startIndex % 128;
      uint64_t a = _mm_extract_epi64(m_data[table], 0);
      uint64_t b = _mm_extract_epi64(m_data[table], 1);
      size_t idx = 0;
      size_t innerIdx = 0;
      if (offset < 64)
      {
        for (size_t i = 0; i < offset; ++i)
        {
          a &= ~(1LL << (i));
        }
        innerIdx = __builtin_ctzll(a);
        while (a)
        {
          ++idx;
          a &= ~(1LL << (innerIdx));
          innerIdx = __builtin_ctzll(a);
        }
      }
      for (size_t i = 0; i < offset - 64; ++i)
      {
        b &= ~(1LL << (i));
      }
      innerIdx = __builtin_ctzll(b);
      while (b)
      {
        ++idx;
        b &= ~(1LL << (innerIdx));
        innerIdx = __builtin_ctzll(b);
      }
      return idx;
    }
  };*/

  class DynamicBitfield
  {
    static const int BitCount = 128;
    std::vector<__m128i> m_data;
    size_t m_size;
    size_t m_pages;

    void growTo(size_t dataIndex)
    {
      while (m_pages <= dataIndex)
      {
        m_data.emplace_back(_mm_setzero_si128());
        m_pages++;
      }
    }
  public:
    DynamicBitfield()
      : m_size(0)
      , m_pages(0)
    {
    }

    DynamicBitfield(size_t size)
      : m_size(size)
      , m_pages(0)
    {
      auto pages = size / BitCount;
      growTo(pages);

      bitfieldUtils::setZero(m_data.data(), static_cast<int>(m_pages));
    }
    DynamicBitfield(const std::vector<__m128i>& data)
      : m_data(data)
      , m_size(data.size() * BitCount)
      , m_pages(m_data.size())
    {
    }

    void initFull()
    {
      bitfieldUtils::setFull(m_data.data(), static_cast<int>(m_pages));
    }

    inline size_t size() const
    {
      return m_pages * BitCount;
    }

    inline size_t sizeBytes() const
    {
      return m_pages * (BitCount/8);
    }

    inline __m128i* data() noexcept
    {
      return m_data.data();
    }

    inline const __m128i* data() const noexcept
    {
      return m_data.data();
    }

    inline void setBit(size_t index)
    {
      auto dataIndex = index / BitCount;
      growTo(dataIndex);
      auto subIndex = index - dataIndex * BitCount;
      bitOps::setbit(m_data[dataIndex], subIndex);
    }

    inline bool checkBit(size_t index)
    {
      auto dataIndex = index / BitCount;
      if (dataIndex >= m_pages)
        return false;
      auto subIndex = index - dataIndex * BitCount;
      return bitOps::checkbit(m_data[dataIndex], subIndex);
    }

    inline void toggleBit(size_t index)
    {
      auto dataIndex = index / BitCount;
      growTo(dataIndex);
      auto subIndex = index - dataIndex * BitCount;
      return bitOps::togglebit(m_data[dataIndex], subIndex);
    }
    inline void clearBit(size_t index)
    {
      auto dataIndex = index / BitCount;
      growTo(dataIndex);
      auto subIndex = index - dataIndex * BitCount;
      return bitOps::clearbit(m_data[dataIndex], subIndex);
    }

    inline void difference(const DynamicBitfield& other)
    {
      auto maxSize = std::max(m_pages, other.m_pages);
      auto interSize = std::min(m_pages, other.m_pages);
      //DynamicBitfield result(maxSize * BitCount);
      const __m128i* vec0 = data();
      const __m128i* vec1 = other.data();
      __m128i* p = data();
      for (size_t i = 0; i < interSize; ++i)
      {
        _mm_store_si128(p+i,_mm_and_si128(vec0[i], vec1[i]));
      }
      if (m_pages > other.m_pages)
      {
        for (size_t i = interSize; i < maxSize; ++i)
        {
          _mm_store_si128(p+i, _mm_setzero_si128());
        }
      }
    }

    inline DynamicBitfield intersectFields(const DynamicBitfield& other)
    {
      auto maxSize = std::max(m_pages, other.m_pages);
      auto interSize = std::min(m_pages, other.m_pages);
      DynamicBitfield result(maxSize * BitCount);
      const __m128i* vec0 = data();
      const __m128i* vec1 = other.data();
      __m128i* p = reinterpret_cast<__m128i*>(result.data());
      for (size_t i = 0; i < interSize; ++i)
      {
        _mm_store_si128(p+i,_mm_and_si128(vec0[i], vec1[i]));
      }
      return result;
    }

    inline void subtract(const DynamicBitfield& other)
    {
      auto interSize = std::min(m_pages, other.m_pages);
      const __m128i* vec1 = data();
      const __m128i* vec0 = other.data();
      __m128i* p = data();
      for (size_t i = 0; i < interSize; ++i)
      {
        _mm_store_si128(p+i,_mm_andnot_si128(vec0[i], vec1[i]));
      }
    }

    inline DynamicBitfield exceptFields(const DynamicBitfield& other)
    {
      auto maxSize = std::max(m_pages, other.m_pages);
      auto interSize = std::min(m_pages, other.m_pages);
      DynamicBitfield result(maxSize*BitCount);
      if (interSize == 0)
        return *this; 
      const __m128i* vec1 = data();
      const __m128i* vec0 = other.data();
      __m128i* p = reinterpret_cast<__m128i*>(result.data());
      for (size_t i = 0; i < interSize; ++i)
      {
        _mm_store_si128(p+i,_mm_andnot_si128(vec0[i], vec1[i]));
      }
      if (m_pages >= other.m_pages)
      {
        for (size_t i = interSize; i < maxSize; ++i)
        {
          _mm_store_si128(p+i, vec1[i]);
        }
      }
      return result;
    }

    inline void add(const DynamicBitfield& other)
    {
      if (other.m_pages > m_pages)
        growTo(other.m_pages-1);
      auto unionSize = std::min(m_pages, other.m_pages);
      const __m128i* vec0 = data();
      const __m128i* vec1 = other.data();
      __m128i* p = data();
      for (size_t i = 0; i < unionSize; ++i)
      {
        _mm_store_si128(p+i,_mm_or_si128(vec0[i], vec1[i]));
      }
    }

    inline DynamicBitfield unionFields(const DynamicBitfield& other)
    {
      auto maxSize = std::max(m_pages, other.m_pages);
      auto unionSize = std::min(m_pages, other.m_pages);
      DynamicBitfield result;
      if (m_pages > other.m_pages)
      {
        result = DynamicBitfield(m_data);
      }
      else
      {
        result = DynamicBitfield(other.m_data);
      }
      const __m128i* vec0 = data();
      const __m128i* vec1 = other.data();
      __m128i* p = reinterpret_cast<__m128i*>(result.data());
      for (size_t i = 0; i < unionSize; ++i)
      {
        _mm_store_si128(p+i,_mm_or_si128(vec0[i], vec1[i]));
      }
      return result;
    }

    inline size_t setBits() const
    {
      size_t res = 0;
      for (size_t i = 0; i < m_pages; ++i)
      {
        res += bitOps::popcount(m_data[i]);
      }
      return res;
    }

    int findFirstSetBit(int index) const
    {
      int dataIndex = index / BitCount;
      while(dataIndex < m_pages)
      {
        if (_mm_test_all_zeros(m_data[dataIndex], _mm_setzero_si128()) != 0)
        {
          // found some bits
          int offset = std::max(0, index - dataIndex * BitCount);
          int offset_tmp = 0;
          uint64_t a = _mm_extract_epi64(m_data[dataIndex], 0);
          uint64_t b = _mm_extract_epi64(m_data[dataIndex], 1);

          if (offset >= 64) // only b to check
          {
            offset_tmp = std::min(64, static_cast<int>(offset) - 64);
            // clean set bits before checking what is next.
            for (size_t i = 0; i < offset_tmp; ++i)
            {
              b ^= (-0LL ^ b) & (1LL << i);
            }
            if (b)
            {
              auto ctzll = calcCtzll(b);
              return ctzll + 64 + dataIndex * BitCount;
            }
          }
          else
          {
            // both a and b to check
            offset_tmp = offset;
            for (size_t i = 0; i < offset_tmp; ++i)
            {
              a ^= (-0LL ^ a) & (1LL << i); // zeroes set bits
            }

            if (a)
            {
              auto ctzll = calcCtzll(a);
              return ctzll + dataIndex * BitCount;
            }
            else if (b)
            {
              auto ctzll = calcCtzll(b);
              return ctzll + 64 + dataIndex * BitCount;
            }
          }
        }
        dataIndex++;
      }
      return -1;
    }

    template <typename Func>
    void foreach(Func func)
    {
      auto index = findFirstSetBit(0);
      while(index >= 0)
      {
        func(index);
        index = findFirstSetBit(index+1);
      }
    }

    void findNotSetRange(int notSetBitCount = 10)
    {
      int di = 0;
      int bitsToFind = notSetBitCount;
      int cdi = 0;
      while(di < m_pages)
      {
        ++di;
      }
    }
  };
}

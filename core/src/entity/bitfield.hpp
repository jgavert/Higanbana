#pragma once
#include <array>
#include <vector>
#include <algorithm>
#include <emmintrin.h>
#include <smmintrin.h>
#include <tmmintrin.h>
#include <nmmintrin.h>
#ifdef _MSC_VER
#include <intrin.h>
  inline unsigned long long __builtin_ctzll(unsigned long long x) { unsigned long r; _BitScanForward64(&r, x); return r; }
#endif



  namespace faze
  {
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

    namespace bitOps
    {
      bool checkbit(const __m128i& vec, int n);
      void setbit(__m128i& vec, int n);
      void clearbit(__m128i& vec, int n);
      void togglebit(__m128i& vec, int n);
      size_t popcount(const __m128i& v);
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
        size_t inner_idx = __builtin_ctzll(a);
        if (a)
        {
          return inner_idx + offset;
        }
        else if (b)
        {
          inner_idx = __builtin_ctzll(b);
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
          offset_tmp = (std::min)(64, static_cast<int>(offset)-64);
          for (size_t i = 0; i < offset_tmp; ++i)
          {
            b ^= (-0LL ^ b) & (1LL << i);
          }
          if (b)
          {
            inner_idx = __builtin_ctzll(b);
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
          inner_idx = __builtin_ctzll(a);
          return inner_idx + blockOffset;
        }
        else if (b)
        {
          inner_idx = __builtin_ctzll(b);
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
        size_t inner_idx = __builtin_ctzll(a);
        while (a)
        {
          table[idx++] = inner_idx + offset;
          a &= ~(1LL << (inner_idx));
          inner_idx = __builtin_ctzll(a);
        }
        inner_idx = __builtin_ctzll(b);
        while (b)
        {
          table[idx++] = inner_idx + 64 + offset;
          b &= ~(1LL << (inner_idx));
          inner_idx = __builtin_ctzll(b);
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
        size_t inner_idx = __builtin_ctzll(a);
        while (a)
        {
          size_t value = inner_idx + offset;
          table[idx] = value;
          ++idx;
          pre(value);
          a &= ~(1LL << (inner_idx));
          inner_idx = __builtin_ctzll(a);
        }
        inner_idx = __builtin_ctzll(b);
        while (b)
        {
          size_t value = inner_idx + offset + 64;
          table[idx] = value;
          ++idx;
          pre(value);
          b &= ~(1LL << (inner_idx));
          inner_idx = __builtin_ctzll(b);
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

    class DynamicBitfield
    {
      std::vector<__m128i> m_data;
      size_t m_size;
    public:
      DynamicBitfield()
        : m_size(0)
      {
      }

      DynamicBitfield(size_t size)
        : m_size(size)
      {
        auto pages = size / 128;
        pages += ((size % 128 != 0) ? 1 : 0);

        for (size_t i = 0; i < pages; ++i)
          m_data.emplace_back(_mm_setzero_si128());
        bitfieldUtils::setZero(m_data.data(), static_cast<int>(pages));
      }
      DynamicBitfield(const std::vector<__m128i>& data)
        : m_data(data)
        , m_size(data.size()*128)
      {
      }

      void initFull()
      {
        bitfieldUtils::setFull(m_data.data(), static_cast<int>(m_data.size()));
      }

      inline size_t size() const
      {
        return m_size;
      }

      inline void setBit(size_t index)
      {
        bitOps::setbit(m_data[index / 128], index % 128);
      }

      inline bool checkBit(size_t index)
      {
        return bitOps::checkbit(m_data[index / 128], index % 128);
      }

      inline void toggleBit(size_t index)
      {
        return bitOps::togglebit(m_data[index / 128], index % 128);
      }
      inline void clearBit(size_t index)
      {
        return bitOps::clearbit(m_data[index / 128], index % 128);
      }

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
          for (int i=0; i < offset; ++i)
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
        for (int i = 0; i < offset-64; ++i)
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
    };
  }

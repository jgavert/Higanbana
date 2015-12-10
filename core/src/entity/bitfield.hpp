#pragma once
#include <array>
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
    template <size_t rsize>
    class Bitfield
    {
    private:
      std::array<__m128i, rsize> m_table;

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

      inline bool checkbit(const __m128i& vec, int n)
      {
        __m128i shuf = _mm_set_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
        shuf = _mm_add_epi8(shuf, _mm_set1_epi8(16 - (static_cast<char>(n) >> 3)));
        shuf = _mm_and_si128(shuf, _mm_set1_epi8(0x0F));
        __m128i setmask = _mm_shuffle_epi8(_mm_cvtsi32_si128(1 << (n & 0x7)), shuf);
        return (_mm_testc_si128(vec, setmask) > 0);
      }

      inline void setbit(__m128i& vec, int n)
      {
        __m128i shuf = _mm_set_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
        shuf = _mm_add_epi8(shuf, _mm_set1_epi8(16 - (static_cast<char>(n) >> 3)));
        shuf = _mm_and_si128(shuf, _mm_set1_epi8(0x0F));
        __m128i setmask = _mm_shuffle_epi8(_mm_cvtsi32_si128(1 << (n & 0x7)), shuf);
        vec = _mm_or_si128(vec, setmask);
      }

      inline void clearbit(__m128i& vec, int n)
      {
        __m128i shuf = _mm_set_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
        shuf = _mm_add_epi8(shuf, _mm_set1_epi8(16 - (n >> 3)));
        shuf = _mm_and_si128(shuf, _mm_set1_epi8(0x0F));
        __m128i setmask = _mm_shuffle_epi8(_mm_cvtsi32_si128(1 << (n & 0x7)), shuf);
        setmask = _mm_andnot_si128(setmask, _mm_sub_epi64(_mm_set_epi64x(0LL, 0LL), _mm_set_epi64x(1LL, 1LL)));
        vec = _mm_and_si128(vec, setmask);
      }

      inline void togglebit(__m128i& vec, int n)
      {
        __m128i shuf = _mm_set_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
        shuf = _mm_add_epi8(shuf, _mm_set1_epi8(16 - (n >> 3)));
        shuf = _mm_and_si128(shuf, _mm_set1_epi8(0x0F));
        __m128i setmask = _mm_shuffle_epi8(_mm_cvtsi32_si128(1 << (n & 0x7)), shuf);
        vec = _mm_xor_si128(vec, setmask);
      }
      inline size_t popcount(const __m128i& v) const
      {
        return _mm_popcnt_u64(_mm_extract_epi64(v, 0)) + _mm_popcnt_u64(_mm_extract_epi64(v, 1));
      }

      size_t countOneBlocks(const std::array<__m128i, rsize>& vec) const
      {
        size_t res = 0;
        const long long vl = std::numeric_limits<long long>::max();
        const __m128i mask = { vl, vl };
        for (size_t i = 0; i < rsize; ++i)
        {
          res += !_mm_test_all_zeros(vec[i], mask);
        }
        return res;
      }

      size_t countPopulatedBlocks(const std::array<__m128i, rsize>& vec) const
      {
        size_t res = 0;
        for (size_t i = 0; i < rsize; ++i)
        {
          res += (popcount(vec[i]) > 0);
        }
        return res;
      }

    public:
      Bitfield() :m_table(init()) {}
      Bitfield(bool) :m_table(init_full()) {}
      Bitfield(std::array<__m128i, rsize> data) :m_table(data) {}

      inline size_t size() const
      {
        return rsize;
      }

      inline void setIdxBit(size_t n)
      {
        setbit(m_table[n / 128], n % 128);
      }

      inline bool checkIdxBit(size_t n)
      {
        return checkbit(m_table[n / 128], n % 128);
      }

      inline void toggleIdxBit(size_t n)
      {
        return togglebit(m_table[n / 128], n % 128);
      }
      inline void clearIdxBit(size_t n)
      {
        return clearbit(m_table[n / 128], n % 128);
      }

      inline size_t nextPopBucket(size_t index) const
      {
        while (index < rsize && popcount(m_table[index]) == 0)
        {
          ++index;
        }
        return index;
      }

      inline size_t skip_find_firstEmpty(size_t block) const
      {
        uint64_t a = _mm_extract_epi64(m_table[block], 0);
        uint64_t b = _mm_extract_epi64(m_table[block], 1);
        size_t offset = block * 128;
        size_t inner_idx = __builtin_ctzll(a);
        size_t expectedEmpty = 0; // this will be uninitialized value if default value is "inner_idx". wtf!
        while (a)
        {
          //table[idx++] = inner_idx + offset;
          if (inner_idx > expectedEmpty)
          {
            return offset + expectedEmpty;
          }
          a &= ~(1LL << (inner_idx));
          ++expectedEmpty;
          inner_idx = __builtin_ctzll(a);
        }
        if (inner_idx+64 > expectedEmpty)
        {
          return offset + expectedEmpty;
        }
        inner_idx = __builtin_ctzll(b);
        expectedEmpty = inner_idx;
        while (b)
        {
          if (inner_idx > expectedEmpty)
          {
            return offset + expectedEmpty + 64;
          }
          b &= ~(1LL << (inner_idx));
          ++expectedEmpty;
          inner_idx = __builtin_ctzll(b);
        }
        if (inner_idx + 128 > expectedEmpty)
        {
          return offset + 64 + expectedEmpty;
        }
        return offset + inner_idx + 64;
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
          res += popcount(m_table[i]);
        }
        return res;
      }

      inline size_t contiguous_full_buckets(size_t index) const
      {
        size_t count = 0;
        while (index < rsize && popcount(m_table[index]) == 128)
        {
          ++count;
          ++index;
        }
        return count;
      }

      inline size_t popcount_element(size_t index) const
      {
        return _mm_popcnt_u64(_mm_extract_epi64(m_table[index], 0)) + _mm_popcnt_u64(_mm_extract_epi64(m_table[index], 1));
      }
    };
  }

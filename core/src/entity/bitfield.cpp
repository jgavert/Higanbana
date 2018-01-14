#include "bitfield.hpp"



  namespace faze
  {
    namespace bitfieldUtils
    {
      void setZero(__m128i* data, int count)
      {
        for (size_t i = 0; i < static_cast<size_t>(count); ++i)
        {
          _mm_store_si128(data + i, _mm_setzero_si128());
        }
      }

      void setFull(__m128i* data, int count)
      {
        for (size_t i = 0; i < static_cast<size_t>(count); ++i)
        {
          _mm_store_si128(data + i, _mm_sub_epi64(_mm_set_epi64x(0LL, 0LL), _mm_set_epi64x(1LL, 1LL)));
        }
      }
    }

    namespace bitOps
    {
      bool checkbit(const __m128i& vec, int n)
      {
        __m128i shuf = _mm_set_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
        shuf = _mm_add_epi8(shuf, _mm_set1_epi8(16 - (static_cast<char>(n) >> 3)));
        shuf = _mm_and_si128(shuf, _mm_set1_epi8(0x0F));
        __m128i setmask = _mm_shuffle_epi8(_mm_cvtsi32_si128(1 << (n & 0x7)), shuf);
        return (_mm_testc_si128(vec, setmask) > 0);
      }

      void setbit(__m128i& vec, int n)
      {
        __m128i shuf = _mm_set_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
        shuf = _mm_add_epi8(shuf, _mm_set1_epi8(16 - (static_cast<char>(n) >> 3)));
        shuf = _mm_and_si128(shuf, _mm_set1_epi8(0x0F));
        __m128i setmask = _mm_shuffle_epi8(_mm_cvtsi32_si128(1 << (n & 0x7)), shuf);
        vec = _mm_or_si128(vec, setmask);
      }

      void clearbit(__m128i& vec, int n)
      {
        __m128i shuf = _mm_set_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
        shuf = _mm_add_epi8(shuf, _mm_set1_epi8(static_cast<char>(16 - (static_cast<uint8_t>(n) >> 3))));
        shuf = _mm_and_si128(shuf, _mm_set1_epi8(0x0F));
        __m128i setmask = _mm_shuffle_epi8(_mm_cvtsi32_si128(1 << (static_cast<uint8_t>(n) & 0x7)), shuf);
        setmask = _mm_andnot_si128(setmask, _mm_sub_epi64(_mm_set_epi64x(0LL, 0LL), _mm_set_epi64x(1LL, 1LL)));
        vec = _mm_and_si128(vec, setmask);
      }

      void togglebit(__m128i& vec, int n)
      {
        __m128i shuf = _mm_set_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
        shuf = _mm_add_epi8(shuf, _mm_set1_epi8(16 - (static_cast<uint8_t>(n) >> 3)));
        shuf = _mm_and_si128(shuf, _mm_set1_epi8(0x0F));
        __m128i setmask = _mm_shuffle_epi8(_mm_cvtsi32_si128(1 << (static_cast<uint8_t>(n) & 0x7)), shuf);
        vec = _mm_xor_si128(vec, setmask);
      }
      size_t popcount(const __m128i& v)
      {
        return _mm_popcnt_u64(_mm_extract_epi64(v, 0)) + _mm_popcnt_u64(_mm_extract_epi64(v, 1));
      }
    }
  }

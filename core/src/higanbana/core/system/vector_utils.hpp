#pragma once
#include <emmintrin.h>
#include <smmintrin.h>
#include <tmmintrin.h>
#include <nmmintrin.h>
#ifdef _MSC_VER
#include <intrin.h>
#endif

#include "higanbana/core/global_debug.hpp"

namespace higanbana
{
inline void cache_bypass_memcpy(void* dst, const void* src, size_t num)
{
  HIGAN_ASSERT(reinterpret_cast<size_t>(dst) % 16 == 0 && num % 16 == 0 && reinterpret_cast<size_t>(src) % 16 == 0, "Src and Dst and datasize has to be aligned to 16 for this to work.");
  __m128i* vecdst = reinterpret_cast<__m128i*>(dst);
  const __m128i* vecsrc = reinterpret_cast<const __m128i*>(src);
  const size_t times = num >> 4;
  for (int i = 0; i < times; ++i)
  {
    _mm_stream_si128(vecdst+i, vecsrc[i]);
  }
}
}
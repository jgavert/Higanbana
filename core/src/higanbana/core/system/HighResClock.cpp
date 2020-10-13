#include "higanbana/core/system/HighResClock.hpp"

#ifdef HIGANBANA_PLATFORM_WINDOWS

#include <Windows.h>

// for some reason
// g_Frequency isnt initialized and results in "0" which caused problems. wtf
// so this is now not used
namespace
{
  const long long g_Frequency = []() -> long long
  {
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    return frequency.QuadPart;
  }();
}

namespace higanbana
{
  HighResClock::time_point HighResClock::now()
  {
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    return time_point(duration(count.QuadPart * static_cast<rep>(period::den) / frequency.QuadPart));
  }

  HighResClock::time_point HighResClock::fromPerfCounter(uint64_t count)
  {
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    LONGLONG dur = static_cast<LONGLONG>(count);
    return time_point(duration(dur * static_cast<rep>(period::den) / frequency.QuadPart));
  }
};
#endif

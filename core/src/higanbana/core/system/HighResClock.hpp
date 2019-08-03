#pragma once
#include "higanbana/core/platform/definitions.hpp"
#ifdef HIGANBANA_PLATFORM_WINDOWS
#include <chrono>

namespace higanbana
{
  struct HighResClock
  {
    typedef long long                               rep;
    typedef std::nano                               period;
    typedef std::chrono::duration<rep, period>      duration;
    typedef std::chrono::time_point<HighResClock>   time_point;
    static const bool is_steady = true;

    static time_point now();
  };
};
using HighPrecisionClock = higanbana::HighResClock;
using timepoint = std::chrono::time_point<higanbana::HighResClock>;
#else
using HighPrecisionClock = std::chrono::high_resolution_clock;
using timepoint = std::chrono::high_resolution_clock::time_point;
#endif

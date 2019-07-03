#pragma once
#include "higanbana/core/global_debug.hpp"
#include <iostream>
#include <chrono>
#include <functional>

#ifdef HIGANBANA_PLATFORM_WINDOWS
#include "higanbana/core/system/HighResClock.hpp"
typedef HighResClock HighPrecisionClock;
typedef std::chrono::time_point<HighResClock> timepoint;
#else
typedef std::chrono::high_resolution_clock HighPrecisionClock;
typedef std::chrono::high_resolution_clock::time_point timepoint;
#endif

namespace higanbana
{
  // Because normal names like 'Benchmarker' or 'Benchmarksystem' are bit boring.
  class Bentsumaakaa
  {
  public:
    Bentsumaakaa();
    ~Bentsumaakaa();
    void start(const bool verbose = false);
    int64_t stop(const bool verbose = false);
    int64_t bfunc(const int times, const bool verbose, const std::function<void()> block);

    template<int times>
    int64_t bfunc2(const bool verbose, const std::function<void()> block)
    {
      if (verbose)
        F_SLOG("Bentsumaakaa", "Benchmarking function for %d times.\n", times);
      auto point1 = std::chrono::high_resolution_clock::now();
      for (int i = 0; i < times; i++)
      {
        block();
      }
      auto point2 = std::chrono::high_resolution_clock::now();
      int64_t time = std::chrono::duration_cast<std::chrono::nanoseconds>(point2 - point1).count();
      if (verbose)
        F_SLOG("Bentsumaakaa", "Benchmarking finished, average time was %.3f ms.\n", static_cast<float>((time / static_cast<int64_t>(times))) / 1000000.f);
      return time / static_cast<int64_t>(times);
    }

  private:
    timepoint startpoint;
  };

}


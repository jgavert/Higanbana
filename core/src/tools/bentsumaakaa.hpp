#pragma once
#include <iostream>
#include <chrono>
#include <functional>

#ifdef WIN64
#include "../system/HighResClock.hpp"
typedef HighResClock HighPrecisionClock;
typedef std::chrono::time_point<HighResClock> timepoint;
#else
typedef std::chrono::high_resolution_clock HighPrecisionClock;
typedef std::chrono::high_resolution_clock::time_point timepoint;
#endif

namespace faze
{
  // Because normal names like 'Benchmarker' or 'Benchmarksystem' are bit boring.
  class Bentsumaakaa
  {
  public:
    Bentsumaakaa();
    ~Bentsumaakaa();
    void start(const bool verbose);
    int64_t stop(const bool verbose);
    int64_t bfunc(const int times, const bool verbose, const std::function<void()> block);

    template<int times>
    int64_t bfunc2(const bool verbose, const std::function<void()> block)
    {
      if (verbose)
        std::cout << "Bentsumaakaa: Benchmarking function for " << times << " times." << std::endl;
      auto point1 = std::chrono::high_resolution_clock::now();
      for (int i = 0; i < times; i++)
      {
        block();
      }
      auto point2 = std::chrono::high_resolution_clock::now();
      int64_t time = std::chrono::duration_cast<std::chrono::nanoseconds>(point2 - point1).count();
      if (verbose)
        std::cout << "Bentsumaakaa: Benchmarking finished, average time was " << static_cast<float>((time / static_cast<int64_t>(times))) / 1000000.f << "ms." << std::endl;
      return time / static_cast<int64_t>(times);
    }

  private:
    timepoint startpoint;
  };

}


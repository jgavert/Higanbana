#include "core/tools/bentsumaakaa.hpp"

using namespace faze;

Bentsumaakaa::Bentsumaakaa()
{
}
Bentsumaakaa::~Bentsumaakaa()
{
}
void Bentsumaakaa::start(const bool verbose)
{
  if (verbose)
    std::cout << "Bentsumaakaa: Start()\n";
  startpoint = HighPrecisionClock::now();
}
int64_t Bentsumaakaa::stop(const bool verbose)
{
  auto endpoint = HighPrecisionClock::now();
  int64_t time = std::chrono::duration_cast<std::chrono::nanoseconds>(endpoint - startpoint).count();
  if (verbose)
    std::cout << "Bentsumaakaa: stop(), time was " << time/1000000.f << "ms.\n";
  return time;
}

int64_t Bentsumaakaa::bfunc(const int times, const bool verbose, const std::function<void()> block)
{
  if (verbose)
    std::cout << "Bentsumaakaa: Benchmarking function for " << times << " times."<< std::endl;
  auto point1 = std::chrono::high_resolution_clock::now();
  for (int i=0;i<times;i++) {
    block();
  }
  auto point2 = std::chrono::high_resolution_clock::now();
  int64_t time = std::chrono::duration_cast<std::chrono::nanoseconds>(point2 - point1).count();
  if (verbose)
    std::cout << "Bentsumaakaa: Benchmarking finished, average time was " << static_cast<float>((time/static_cast<int64_t>(times)))/1000000.f << " ms." << std::endl;
  return time/static_cast<int64_t>(times);
}



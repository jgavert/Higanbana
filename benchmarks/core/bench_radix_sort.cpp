#include <catch2/catch_all.hpp>

#include <higanbana/core/profiling/profiling.hpp>
#include <higanbana/core/sort/radix_sort.hpp>
#include <higanbana/core/sort/radix_sort_coro.hpp>

#include <random>


TEST_CASE("Benchmark radix sort", "[benchmark]") {
  using namespace higanbana;
  css::createThreadPool();
  std::mt19937 gen32;
  vector<unsigned> srcData;
  srcData.reserve(1000000ull);
  for (size_t i = 0; i < 1000000ull; i++) {
    srcData.push_back(gen32());
  }
  auto ref = srcData;
  std::sort(ref.begin(), ref.end());


  BENCHMARK("std") {
    auto copy = srcData;
    std::sort(copy.begin(), copy.end());
    return copy[123];
  };

  BENCHMARK("baseline - 1 Thread") {
    auto copy = srcData;
    radix_sort_8bits(copy);
    return copy[123];
  };
  BENCHMARK("task - 1 Thread") {
    auto copy = srcData;
    radix_sort_task<8, 1>(copy).wait();
    return copy[123];
  };
  /*
  BENCHMARK("task fast - 1 Thread 10 Million") {
    auto copy = srcData;
    radix_sort_task_fast<8, 1>(copy).wait();
    return copy[123];
  };
  BENCHMARK("task fast - 2 Thread 10 Million") {
    auto copy = srcData;
    radix_sort_task_fast<8, 2>(copy).wait();
    return copy[123];
  };
  BENCHMARK("task fast - 4 Thread 10 Million") {
    auto copy = srcData;
    radix_sort_task_fast<8, 4>(copy).wait();
    return copy[123];
  };
  BENCHMARK("task fast - 6 Thread 10 Million") {
    auto copy = srcData;
    radix_sort_task_fast<8, 6>(copy).wait();
    return copy[123];
  };
  */
  BENCHMARK("task fast - 8 Thread") {
    auto copy = srcData;
    radix_sort_task_fast<8, 8>(copy).wait();
    return copy[123];
  };
  BENCHMARK("task fast - 32 Thread") {
    auto copy = srcData;
    radix_sort_task_fast<8, 32>(copy).wait();
    return copy[123];
  };
  BENCHMARK("task fast 12bit - 8 Thread") {
    auto copy = srcData;
    radix_sort_task_fast<12, 8>(copy).wait();
    return copy[123];
  };
  BENCHMARK("task fast 12bit - 32 Thread") {
    auto copy = srcData;
    radix_sort_task_fast<12, 32>(copy).wait();
    return copy[123];
  };
  BENCHMARK("task fast 11bit - 8 Thread") {
    auto copy = srcData;
    radix_sort_task_fast<11, 8>(copy).wait();
    return copy[123];
  };
}
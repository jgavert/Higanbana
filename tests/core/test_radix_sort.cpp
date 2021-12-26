#include <catch2/catch_all.hpp>
#include <higanbana/core/sort/radix_sort.hpp>
#include <higanbana/core/sort/radix_sort_coro.hpp>
#include <vector>

void radix_sort_10(std::vector<unsigned>& data) {
  auto copy = data;
  unsigned counts[10]{};
  // count digits
  for (unsigned i = 0; i < data.size(); i++) {
    counts[data[i]]++;
  }
  // prefix sum
  unsigned sum = 0;
  for (unsigned i = 0; i < 10; i++) {
    counts[i] += sum;
    sum = counts[i];
  }
  // write copy 
  for (unsigned i = data.size(); i > 0; i--) {
    int index = --counts[data[i-1]];
    copy[index] = data[i-1];
  }
  // write output
  std::copy(copy.begin(), copy.end(), data.begin());
}

void radix_sort_16(std::vector<unsigned>& data) {
  auto copy = data;
  unsigned counts[16]{};
  // count digits
  for (unsigned i = 0; i < data.size(); i++) {
    counts[data[i]&(16-1)]++;
  }
  // prefix sum
  unsigned sum = 0;
  for (unsigned i = 0; i < 16; i++) {
    counts[i] += sum;
    sum = counts[i];
  }
  // write copy 
  for (unsigned i = data.size(); i > 0; i--) {
    int index = --counts[data[i-1]];
    copy[index] = data[i-1];
  }
  // write output
  std::copy(copy.begin(), copy.end(), data.begin());
}


TEST_CASE( "radix sort 10 single numbers" ) {
  std::vector<unsigned> data{5,4,0,6,8,7,2,3,9,8,1};
  auto control = data;
  std::sort(control.begin(), control.end());
  radix_sort_10(data);

  for (unsigned i = 0; i < 10; i++) {
    REQUIRE(control[i] == data[i]);
  }
}

TEST_CASE( "radix sort 16" ) {
  std::vector<unsigned> data{5,4,0,6,8,7,2,3,9,8,1,10,15,14,13,13,12};
  auto control = data;
  std::sort(control.begin(), control.end());
  radix_sort_16(data);

  for (unsigned i = 0; i < data.size(); i++) {
    REQUIRE(control[i] == data[i]);
  }
}

TEST_CASE( "radix sort 8bit" ) {
  std::vector<unsigned> data{5,12025,50,255, 12459872};
  auto control = data;
  std::sort(control.begin(), control.end());
  higanbana::radix_sort_8bits(data);

  for (unsigned i = 0; i < data.size(); i++) {
    REQUIRE(control[i] == data[i]);
  }
}

TEST_CASE("thread radix sort 8bit") {
  css::createThreadPool();

  std::vector<unsigned> data{5,12025,50,255, 12459872};
  auto control = data;
  std::sort(control.begin(), control.end());

  auto task = higanbana::radix_sort_8bits_task(data);
  task.wait();

  for (unsigned i = 0; i < data.size(); i++) {
    REQUIRE(control[i] == data[i]);
  }
}

TEST_CASE("thread radix sort 8bit simple") {
  css::createThreadPool();

  std::vector<unsigned> data{5,12025,50,255, 12459872};
  auto control = data;
  std::sort(control.begin(), control.end());

  auto task = higanbana::radix_sort_task<11,1>(data);
  task.wait();

  for (unsigned i = 0; i < data.size(); i++) {
    REQUIRE(control[i] == data[i]);
  }
}

TEST_CASE("thread radix sort 8bit 2") {
  css::createThreadPool();

  std::vector<unsigned> data{5,12025,50,255, 12459872};
  for (int i = 0; i < 100000; ++i) {
    data.push_back(300000 - i*3);
  }
  auto control = data;
  std::sort(control.begin(), control.end());

  auto task = higanbana::radix_sort_task<8, 4>(data);
  task.wait();

  for (unsigned i = 0; i < data.size(); i++) {
    REQUIRE(control[i] == data[i]);
  }
}

TEST_CASE("thread radix sort long") {
  css::createThreadPool();

  std::vector<unsigned> data{5,12025,50,255, 12459872};
  for (int i = 0; i < 3000000; ++i) {
    data.push_back(i*3);
  }
  auto control = data;
  std::sort(control.begin(), control.end());

  auto task = higanbana::radix_sort_task<12, 4>(data);
  task.wait();

  for (unsigned i = 0; i < data.size(); i++) {
    REQUIRE(control[i] == data[i]);
  }
}

TEST_CASE("thread radix sort fast long") {
  css::createThreadPool();

  std::vector<unsigned> data{5,12025,50,255, 12459872};
  for (int i = 0; i < 3000000; ++i) {
    data.push_back(i*3);
  }
  auto control = data;
  std::sort(control.begin(), control.end());

  auto task = higanbana::radix_sort_task_fast<12, 8>(data);
  task.wait();

  for (unsigned i = 0; i < data.size(); i++) {
    REQUIRE(control[i] == data[i]);
  }
}
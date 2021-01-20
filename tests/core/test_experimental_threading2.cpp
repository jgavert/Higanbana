#define CATCH_CONFIG_DISABLE_EXCEPTIONS
#include <catch2/catch_all.hpp>

#include <higanbana/core/system/time.hpp>
#include <higanbana/core/profiling/profiling.hpp>

#include <css/task.hpp>

#include <vector>
#include <thread>
#include <future>
#include <optional>
#include <cstdio>
#include <iostream>
#include <deque>
#include <atomic>

#include <windows.h>

css::Task<void> empty_function() {
  co_return;
}

TEST_CASE("c++ threading")
{
  css::createThreadPool();
  empty_function().wait();
}
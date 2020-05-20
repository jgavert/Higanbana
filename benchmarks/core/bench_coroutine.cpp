#include <catch2/catch.hpp>

#include <higanbana/core/profiling/profiling.hpp>
#include <higanbana/core/coroutine/task.hpp>
#include <higanbana/core/coroutine/parallel_task.hpp>
#include <higanbana/core/coroutine/task_st.hpp>
#include <higanbana/core/coroutine/stolen_task.hpp>
#include <higanbana/core/coroutine/reference_coroutine_task.hpp>
#include <higanbana/core/system/LBS.hpp>

#include <vector>
#include <thread>
#include <future>
#include <optional>
#include <cstdio>
#include <iostream>
#include <deque>
#include <experimental/coroutine>
#include <windows.h>
#include <execution>
#include <algorithm>

using namespace higanbana;
using namespace higanbana::experimental;
namespace {

    template<typename T>
    T empty_function() {
      co_return;
    }

    template<typename T>
    T child() {
      co_return 1;
    }

    uint64_t Fibonacci(uint64_t number) noexcept {
      return number < 2 ? 1 : Fibonacci(number - 1) + Fibonacci(number - 2);
    }
    
    uint64_t fibonacciLoop(uint64_t nthNumber) noexcept {
      uint64_t previouspreviousNumber, previousNumber = 0, currentNumber = 1;
      for (uint64_t i = 1; i < nthNumber+1 ; i++) {
        previouspreviousNumber = previousNumber;
        previousNumber = currentNumber;
        currentNumber = previouspreviousNumber + previousNumber;
      }
      return currentNumber;
    }
    reference::Task<uint64_t> FibonacciReferenceIterative(uint64_t number) noexcept {
      co_return fibonacciLoop(number);
    }

    reference::Task<uint64_t> FibonacciReferenceRecursive(uint64_t number) noexcept {
      co_return Fibonacci(number);
    }
    template<typename T>
    T FibonacciCoro(uint64_t number) noexcept {
      if (number < 2)
        co_return 1;
        
      auto v0 = FibonacciCoro<T>(number-1);
      auto v1 = FibonacciCoro<T>(number-2);
      co_return co_await v0 + co_await v1;
    }
    template<typename T>
    T SpawnEmptyTasksInTree(uint64_t tasks) noexcept {
      if (tasks <= 1)
        co_return;
      tasks = tasks - 1;
      if (tasks == 1){
        co_await empty_function<T>();
        co_return;
      }
      
      uint64_t split = tasks / 2;
      uint64_t splitOff = tasks % 2;
      auto v0 = SpawnEmptyTasksInTree<T>(split);
      auto v1 = SpawnEmptyTasksInTree<T>(split + splitOff);
      co_await v0;
      co_await v1;
      co_return;
    }
}

#define BenchFunction(calledFunction, argument) \
    BENCHMARK(#calledFunction ## "(" ## #argument ## ")") { \
        return calledFunction(argument).get(); \
    }
#define BenchFunctionWait(calledFunction, argument) \
    BENCHMARK(#calledFunction ## "(" ## #argument ## ")") { \
        return calledFunction(argument).wait(); \
    }


#define checkAllEmptyTasksSpawning(argument) \
    BenchFunctionWait(SpawnEmptyTasksInTree<reference::Task<void>>, argument); \
    BenchFunctionWait(SpawnEmptyTasksInTree<corost::Task<void>>, argument); \
    BenchFunctionWait(SpawnEmptyTasksInTree<coro::StolenTask<void>>, argument)


#define checkAllFibonacci(argument) \
    BenchFunction(FibonacciReferenceIterative, argument); \
    BenchFunction(FibonacciReferenceRecursive, argument); \
    BenchFunction(FibonacciCoro<reference::Task<uint64_t>>, argument); \
    BenchFunction(FibonacciCoro<corost::Task<uint64_t>>, argument); \
    BenchFunction(FibonacciCoro<coro::StolenTask<uint64_t>>, argument)

TEST_CASE("Benchmark Fibonacci", "[benchmark]") {
    //higanbana::experimental::globals::createGlobalLBSPool();
    higanbana::taskstealer::globals::createTaskStealingPool();
    higanbana::experimental::stts::globals::createSingleThreadPool();
    higanbana::experimental::reference::globals::createExecutor();
    
    CHECK(FibonacciReferenceIterative(0).get() == 1);
    CHECK(FibonacciReferenceIterative(5).get() == 8);
    CHECK(FibonacciReferenceRecursive(0).get() == 1);
    CHECK(FibonacciReferenceRecursive(5).get() == 8);
    CHECK(FibonacciCoro<reference::Task<uint64_t>>(0).get() == 1);
    CHECK(FibonacciCoro<reference::Task<uint64_t>>(5).get() == 8);
    CHECK(FibonacciCoro<corost::Task<uint64_t>>(0).get() == 1);
    CHECK(FibonacciCoro<corost::Task<uint64_t>>(5).get() == 8);
    CHECK(FibonacciCoro<coro::StolenTask<uint64_t>>(0).get() == 1);
    CHECK(FibonacciCoro<coro::StolenTask<uint64_t>>(5).get() == 8);
    checkAllFibonacci(20);
    checkAllEmptyTasksSpawning(100);
    checkAllEmptyTasksSpawning(1000);
    checkAllEmptyTasksSpawning(65000);
    checkAllEmptyTasksSpawning(100000);
    checkAllEmptyTasksSpawning(200000);
    /*
    BENCHMARK("Coroutine Fibonacci 25") {
        return FibonacciCoro(25, 25-parallel+1).get();
    };
    BENCHMARK("Fibonacci 30") {
        return FibonacciOrig(30).get();
    };
    BENCHMARK("Coroutine Fibonacci 30") {
        return FibonacciCoro(30,30-parallel).get();
    };
    
    BENCHMARK("Fibonacci 32") {
        return FibonacciOrig(32).get();
    };
    BENCHMARK("Coroutine Fibonacci 32") {
        return FibonacciCoro(32,32-parallel).get();
    };
    
    BENCHMARK("Fibonacci 36") {
        return FibonacciOrig(36).get();
    };
    BENCHMARK("Coroutine Fibonacci 36") {
        return FibonacciCoro(36,36-parallel-1).get();
    };
    /*
    BENCHMARK("Fibonacci 38") {
        return FibonacciOrig(38).get();
    };
    BENCHMARK("Coroutine Fibonacci 38") {
        return FibonacciCoro(38,38-parallel-3).get();
    };*/
}
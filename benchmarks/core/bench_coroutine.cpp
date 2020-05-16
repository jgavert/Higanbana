#include <catch2/catch.hpp>

#include <higanbana/core/profiling/profiling.hpp>
#include <higanbana/core/coroutine/task.hpp>

#include <vector>
#include <thread>
#include <future>
#include <optional>
#include <cstdio>
#include <iostream>
#include <deque>
#include <experimental/coroutine>
#include <windows.h>


struct suspend_never {
  constexpr bool await_ready() const noexcept { return true; }
  void await_suspend(std::experimental::coroutine_handle<>) const noexcept {}
  constexpr void await_resume() const noexcept {}
};

struct suspend_always {
  constexpr bool await_ready() const noexcept { return false; }
  void await_suspend(std::experimental::coroutine_handle<>) const noexcept {}
  constexpr void await_resume() const noexcept {}
};
template<typename T>
class async_awaitable {
public:
  struct promise_type {
    using coro_handle = std::experimental::coroutine_handle<promise_type>;
    auto get_return_object() {
      //printf("get_return_object\n");
      return coro_handle::from_promise(*this);
    }
    auto initial_suspend() {
      //-printf("initial_suspend\n");
      return suspend_always();
    }
    auto final_suspend() noexcept {
      //printf("final_suspend\n");
      return suspend_always();
    }
    void return_value(T value) {m_value = value;}
    void unhandled_exception() {
      std::terminate();
    }
    auto await_transform(async_awaitable<T> handle) {
      //printf("await_transform\n");
      return handle;
    }
    T m_value;
    std::future<void> m_fut;
    std::promise<void> finalFut;
    std::future<void> m_final;
    std::weak_ptr<coro_handle> weakref;
  };
  using coro_handle = std::experimental::coroutine_handle<promise_type>;
  async_awaitable(coro_handle handle) : handle_(std::shared_ptr<coro_handle>(new coro_handle(handle), [](coro_handle* ptr){ptr->destroy(); delete ptr;}))
  {
    //printf("created async_awaitable\n");
    assert(handle);
    handle_->promise().m_final = handle_->promise().finalFut.get_future();

    handle_->promise().weakref = handle_;
    handle_->promise().m_fut = std::async(std::launch::async, [handlePtr = handle_]
    {
      if (!handlePtr->done()) {
        handlePtr->resume();
        if (handlePtr->done())
          handlePtr->promise().finalFut.set_value();
      }
    });
  }
  async_awaitable(async_awaitable& other) {
    handle_ = other.handle_;
  };
  async_awaitable(async_awaitable&& other) : handle_(std::move(other.handle_)) {
    //printf("moving\n");
    assert(handle_);
    other.handle_ = nullptr;
    //other.m_fut = nullptr;
  }
  // coroutine meat
  T await_resume() noexcept {
    return handle_->promise().m_value;
  }
  bool await_ready() noexcept {
    return handle_->done();
  }

  // enemy coroutine needs this coroutines result, therefore we compute it.
  void await_suspend(coro_handle handle) noexcept {
    handle_->promise().m_fut.wait();
    handle_->promise().m_final.wait();
    //handle.promise().m_fut.wait();
    std::shared_ptr<coro_handle> otherHandle = handle.promise().weakref.lock();
    
    
    auto newfut = std::async(std::launch::async, [handlePtr = otherHandle]() mutable
    {
      if (!handlePtr->done()) {
        handlePtr->resume();
        if (handlePtr->done())
          handlePtr->promise().finalFut.set_value();
      }
    });
    handle.promise().m_fut = std::move(newfut);
  }
  // :o
  ~async_awaitable() { }
  T get()
  {
    //printf("get\n");
    handle_->promise().m_final.wait();
    assert(handle_->done());
    return handle_->promise().m_value; 
  }
private:
  std::shared_ptr<coro_handle> handle_;
};

namespace {
    std::uint64_t Fibonacci(std::uint64_t number) noexcept {
        return number < 2 ? 1 : Fibonacci(number - 1) + Fibonacci(number - 2);
    }

    async_awaitable<uint64_t> FibonacciOrig(uint64_t number) noexcept {
        co_return Fibonacci(number);
    }
    higanbana::coro::task<uint64_t> FibonacciCoro(uint64_t number, uint64_t parallel) noexcept {
        if (number < 2)
            co_return 1;
        
        if (number > parallel) {
            auto v0 = FibonacciCoro(number-1, parallel);
            auto v1 = FibonacciCoro(number-2, parallel);
            co_return co_await v0 + co_await v1;
        }
        auto fib0 = Fibonacci(number - 1);
        auto fib1 = Fibonacci(number - 2);
        co_return fib0 + fib1;
    }

    async_awaitable<uint64_t> FibonacciAsync(uint64_t number, uint64_t parallel) noexcept {
        if (number < 2)
            co_return 1;
        
        if (number > parallel) {
            auto v0 = FibonacciAsync(number-1, parallel);
            auto v1 = FibonacciAsync(number-2, parallel);
            co_return co_await v0 + co_await v1;
        }
        co_return Fibonacci(number - 1) + Fibonacci(number - 2);
    }
}


TEST_CASE("Benchmark Fibonacci", "[benchmark]") {
    CHECK(FibonacciOrig(0).get() == 1);
    // some more asserts..
    CHECK(FibonacciOrig(5).get() == 8);
    // some more asserts..
    higanbana::experimental::globals::createGlobalLBSPool();
    CHECK(FibonacciCoro(0, 0).get() == 1);
    // some more asserts..
    CHECK(FibonacciCoro(5, 5).get() == 8);
    // some more asserts..
    uint64_t parallel = 6;
    BENCHMARK("Fibonacci 25") {
        return FibonacciOrig(25).get();
    };
    BENCHMARK("Coroutine Fibonacci 25") {
        return FibonacciCoro(25, 25-parallel).get();
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

    BENCHMARK("Fibonacci 38") {
        return FibonacciOrig(38).get();
    };
    BENCHMARK("Coroutine Fibonacci 38") {
        return FibonacciCoro(38,38-parallel-2).get();
    };
    
    
    //my_pool->~LBSPool();


  //higanbana::FileSystem fs("/../../tests/data/");
  //higanbana::profiling::writeProfilingData(fs);
}
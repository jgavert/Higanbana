#include <catch2/catch.hpp>

#include <vector>
#include <thread>
#include <future>
#include <optional>
#include <cstdio>
#include <iostream>
#include <experimental/coroutine>

class resumable {
public:
  struct promise_type {
    using coro_handle = std::experimental::coroutine_handle<promise_type>;
    auto get_return_object() {
      return coro_handle::from_promise(*this);
    }
    auto initial_suspend() { return std::experimental::suspend_always(); }
    auto final_suspend() noexcept { return std::experimental::suspend_always(); }
    void return_void() {}
    void unhandled_exception() {
      std::terminate();
    }
  };
  using coro_handle = std::experimental::coroutine_handle<promise_type>;
  resumable(coro_handle handle) : handle_(handle) { assert(handle); }
  resumable(resumable&) = delete;
  resumable(resumable&&) = delete;
  bool resume() {
    if (!handle_.done())
      handle_.resume();
    return !handle_.done();
  }
  ~resumable() { handle_.destroy(); }
private:
  coro_handle handle_;
};

resumable foo(){
  std::cout << "Hello" << std::endl;
  co_await std::experimental::suspend_always();
  std::cout << "Coroutine" << std::endl;
}

TEST_CASE("c++ threading")
{
  resumable res = foo();
  while (res.resume());
}
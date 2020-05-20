#pragma once
#include "higanbana/core/profiling/profiling.hpp"
#include "higanbana/core/datastructures/fixed_size_deque.hpp"
#include <atomic>
#include <vector>
#include <mutex>
#include <thread>
#include <optional>
#include <experimental/coroutine>


namespace higanbana
{
namespace experimental
{
namespace reference 
{
class CoroutineExecutor
{
  std::deque<std::experimental::coroutine_handle<>> stack;
  std::deque<std::experimental::coroutine_handle<>> workQueue;
  public:
  void schedule(std::experimental::coroutine_handle<> handle) {
    workQueue.push_back(handle);
  }
  void reschedule() {
  }

  void execute(std::experimental::coroutine_handle<> ) {
    while(!stack.empty() || !workQueue.empty()) {
      if (workQueue.empty()) {
        auto task = stack.front();
        stack.pop_front();
        task.resume();
        if (!task.done())
          stack.push_front(task);
      }
      else {
        assert(!workQueue.empty());
        auto task = workQueue.back();
        workQueue.pop_back();

        task.resume();
        if (!task.done())
          stack.push_front(task);
      }
    }
  }
};
namespace globals
{
  void createExecutor();
  extern std::unique_ptr<CoroutineExecutor> s_pool;
}
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

template <typename T>
class Task {
public:
  struct promise_type {
    using coro_handle = std::experimental::coroutine_handle<promise_type>;
    auto get_return_object() {
      return coro_handle::from_promise(*this);
    }
    auto initial_suspend() { return suspend_always(); }
    auto final_suspend() noexcept { return suspend_always(); }
    void return_value(T value) noexcept {m_value = std::move(value);}
    void unhandled_exception() {
      std::terminate();
    }
    T m_value;
  };
  using coro_handle = std::experimental::coroutine_handle<promise_type>;
  Task(coro_handle handle) : handle_(handle)
  {
    assert(handle);
    globals::s_pool->schedule(handle);
  }
  Task(Task& other) : handle_(other.handle_) {
    assert(handle_);
  }
  Task(Task&& other) : handle_(std::move(other.handle_)) {
    assert(handle_);
    other.handle_ = nullptr;
  }
  T await_resume() noexcept {
    return handle_.promise().m_value;
  }
  bool await_ready() noexcept {
    return handle_.done();
  }

  // enemy coroutine needs this coroutines result, therefore we compute it.
  void await_suspend(coro_handle) noexcept {
    //globals::s_pool->reschedule();
  }
  T get() noexcept {
    globals::s_pool->execute(handle_);
    return handle_.promise().m_value;
  }

  ~Task() { handle_.destroy(); }
private:
  coro_handle handle_;
};

template <>
class Task<void> {
public:
  struct promise_type {
    using coro_handle = std::experimental::coroutine_handle<promise_type>;
    auto get_return_object() {
      return coro_handle::from_promise(*this);
    }
    auto initial_suspend() { return suspend_always(); }
    auto final_suspend() noexcept { return suspend_always(); }
    void return_void() {}
    void unhandled_exception() {
      std::terminate();
    }
  };
  using coro_handle = std::experimental::coroutine_handle<promise_type>;
  Task(coro_handle handle) : handle_(handle)
  {
    assert(handle);
    globals::s_pool->schedule(handle);
  }
  Task(Task& other) : handle_(other.handle_) {
    assert(handle_);
  }
  Task(Task&& other) : handle_(std::move(other.handle_)) {
    assert(handle_);
    other.handle_ = nullptr;
  }
  void await_resume() noexcept {
  }
  bool await_ready() noexcept {
    return handle_.done();
  }
  void await_suspend(coro_handle) noexcept {
    //globals::s_pool->reschedule();
  }
  void wait() noexcept {
    globals::s_pool->execute(handle_);
  }

  ~Task() { handle_.destroy(); }
private:
  coro_handle handle_;
};
}
}
}
#pragma once
#include "higanbana/core/coroutine/task_stealing_pool.hpp"
#include "higanbana/core/coroutine/coroutine_helpers.hpp"

namespace higanbana
{
namespace coro
{
template<typename T>
class StolenTask {
public:
  struct promise_type {
    using coro_handle = std::experimental::coroutine_handle<promise_type>;
    __declspec(noinline) auto get_return_object() noexcept {
      return StolenTask(coro_handle::from_promise(*this));
    }
    constexpr coro::suspend_always initial_suspend() noexcept {
      return {};
    }

    constexpr coro::suspend_always final_suspend() noexcept {
      return {};
    }
    void return_value(T value) noexcept {m_value = value;}
    void unhandled_exception() noexcept {
      std::terminate();
    }
    T m_value;
  };
  using coro_handle = std::experimental::coroutine_handle<promise_type>;
  StolenTask(coro_handle handle) noexcept : handle_(handle)
  {
    assert(handle_);
    tracker_ = taskstealer::globals::s_stealPool->spawnTask(handle_);
  }
  StolenTask(StolenTask& other) noexcept {
    handle_ = other.handle_;
    tracker_ = other.tracker_;
  };
  StolenTask(StolenTask&& other) noexcept {
    if (other.handle_)
      handle_ = std::move(other.handle_);
    tracker_ = other.tracker_;
    assert(handle_);
    other.handle_ = nullptr;
  }
  StolenTask& operator=(StolenTask& other) noexcept {
    handle_ = other.handle_;
    tracker_ = other.tracker_;
    return *this;
  };
  StolenTask& operator=(StolenTask&& other) noexcept {
    if (other.handle_)
      handle_ = std::move(other.handle_);
    tracker_ = other.tracker_;
    assert(handle_);
    other.handle_ = nullptr;
    return *this;
  }
  T await_resume() noexcept {
    return handle_.promise().m_value;
  }
  bool await_ready() noexcept {
    return handle_.done();
  }

  // enemy coroutine needs this coroutines result, therefore we compute it.
  template <typename Type>
  auto await_suspend(Type handle) noexcept {
    taskstealer::globals::s_stealPool->addDependencyToCurrentTask(handle, handle_, tracker_);
    return coro::noop_coroutine();
  }
  ~StolenTask() noexcept {
    //HIGAN_ASSERT(handle_.done(), "coroutine was destroyed by the creator coroutine before coroutine was completed.");
    //assert(handle_.done());
#if !defined(STEALER_DESTROY_HANDLES)
    if (handle_)
      handle_.destroy();
#endif
  }

  T get() noexcept
  {
    if (!handle_.done())
      taskstealer::globals::s_stealPool->execute();
    auto val = handle_.promise().m_value;
#if defined(STEALER_DESTROY_HANDLES)
    handle_.destroy();
#endif
    return val; 
  }
  bool is_ready() const {
    return handle_.done();
  }
  /*
  bool is_ready() const {
    return handle_ && handle_.done();
  }
  explicit operator bool() const {
    return handle_.address() != nullptr;
  }*/
  // unwrap() future<future<int>> -> future<int>
  // future then(lambda) -> attach function to be done after current Task.
  // is_ready() are you ready?
private:
  std::experimental::coroutine_handle<promise_type> handle_;
  uintptr_t tracker_ = 0;
};

// void version
template <>
class StolenTask<void> {
public:
  struct promise_type {
    using coro_handle = std::experimental::coroutine_handle<promise_type>;
    __declspec(noinline) auto get_return_object() noexcept {
      return StolenTask(coro_handle::from_promise(*this));
    }
    constexpr coro::suspend_always initial_suspend() noexcept {
      return {};
    }

    constexpr coro::suspend_always final_suspend() noexcept {
      return {};
    }
    void return_void() noexcept {}
    void unhandled_exception() noexcept {
      std::terminate();
    }
  };
  using coro_handle = std::experimental::coroutine_handle<promise_type>;
  StolenTask(coro_handle handle) noexcept : handle_(handle)
  {
    assert(handle_);
    tracker_ = taskstealer::globals::s_stealPool->spawnTask(handle_);
  }
  StolenTask(StolenTask& other) noexcept {
    handle_ = other.handle_;
    tracker_ = other.tracker_;
  };
  StolenTask(StolenTask&& other) noexcept {
    if (other.handle_)
      handle_ = std::move(other.handle_);
    tracker_ = other.tracker_;
    assert(handle_);
    other.handle_ = nullptr;
  }
  StolenTask& operator=(StolenTask& other) noexcept {
    handle_ = other.handle_;
    tracker_ = other.tracker_;
    return *this;
  };
  StolenTask& operator=(StolenTask&& other) noexcept {
    if (other.handle_)
      handle_ = std::move(other.handle_);
    tracker_ = other.tracker_;
    assert(handle_);
    other.handle_ = nullptr;
    return *this;
  }
  void await_resume() noexcept {
  }
  bool await_ready() noexcept {
    return handle_.done();
  }

  // enemy coroutine needs this coroutines result, therefore we compute it.
  template <typename Type>
  auto await_suspend(Type handle) noexcept {
    taskstealer::globals::s_stealPool->addDependencyToCurrentTask(handle, handle_, tracker_);
    return coro::noop_coroutine();
  }
  ~StolenTask() noexcept {
    //HIGAN_ASSERT(handle_.done(), "coroutine was destroyed by the creator coroutine before coroutine was completed.");
    //assert(handle_.done());
    //handle_.destroy();
#if !defined(STEALER_DESTROY_HANDLES)
    if (handle_)
      handle_.destroy();
#endif
  }

  void wait() noexcept
  {
    if (!handle_.done())
      taskstealer::globals::s_stealPool->execute();
#if defined(STEALER_DESTROY_HANDLES)
    handle_.destroy();
#endif
  }

  bool is_ready() const {
    return handle_.done();
  }

/*
  explicit operator bool() const {
    return !handle_.done();
  }*/
  // unwrap() future<future<int>> -> future<int>
  // future then(lambda) -> attach function to be done after current Task.
  // is_ready() are you ready?
private:
  std::experimental::coroutine_handle<> handle_;
  uintptr_t tracker_ = 0;
};

template<typename Func>
StolenTask<void> run(Func&& f)
{
  f();
  co_return;
}

}
}
#pragma once
#include "higanbana/core/coroutine/coroutine_pool_st.hpp"
#include "higanbana/core/coroutine/coroutine_helpers.hpp"

namespace higanbana
{
namespace corost
{
template<typename T>
class Task {
public:
  struct promise_type {
    using coro_handle = std::experimental::coroutine_handle<promise_type>;
    __declspec(noinline) auto get_return_object() noexcept {
      return Task(coro_handle::from_promise(*this));
    }
    constexpr coro::suspend_always initial_suspend() noexcept {
      return {};
    }

    constexpr coro::suspend_always final_suspend() noexcept {
      return {};
    }
    void return_value(T value) noexcept {m_value = std::move(value);}
    void unhandled_exception() noexcept {
      std::terminate();
    }
    T m_value;
  };
  using coro_handle = std::experimental::coroutine_handle<promise_type>;
  Task(coro_handle handle) noexcept : handle_(handle)
  {
    experimental::stts::globals::s_pool->spawnTask(handle);
    assert(handle);
  }
  Task(Task& other) noexcept {
    handle_ = other.handle_;
  };
  Task(Task&& other) noexcept {
    if (other.handle_)
      handle_ = std::move(other.handle_);
    assert(handle_);
    other.handle_ = nullptr;
  }
  Task& operator=(Task& other) noexcept {
    handle_ = other.handle_;
    return *this;
  };
  Task& operator=(Task&& other) noexcept {
    if (other.handle_)
      handle_ = std::move(other.handle_);
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
  void await_suspend(Type handle) noexcept {
    experimental::stts::globals::s_pool->addDependencyToCurrentTask(handle_);
  }
  ~Task() noexcept {
    HIGAN_ASSERT(handle_.done(), "coroutine was destroyed by the creator coroutine before coroutine was completed.");
    assert(handle_.done());
    handle_.destroy();
  }

  T get() noexcept
  {
    if (!handle_.done())
      experimental::stts::globals::s_pool->execute();
    return handle_.promise().m_value; 
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
};

// void version
template <>
class Task<void> {
public:
  struct promise_type {
    using coro_handle = std::experimental::coroutine_handle<promise_type>;
    __declspec(noinline) auto get_return_object() noexcept {
      return Task(coro_handle::from_promise(*this));
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
  Task(coro_handle handle) noexcept : handle_(handle)
  {
    assert(handle);
    experimental::stts::globals::s_pool->spawnTask(handle);
  }
  Task(Task& other) noexcept {
    handle_ = other.handle_;
  };
  Task(Task&& other) noexcept {
    if (other.handle_)
      handle_ = std::move(other.handle_);
    assert(handle_);
    other.handle_ = nullptr;
  }
  Task& operator=(Task& other) noexcept {
    handle_ = other.handle_;
    return *this;
  };
  Task& operator=(Task&& other) noexcept {
    if (other.handle_)
      handle_ = std::move(other.handle_);
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
  void await_suspend(Type handle) noexcept {
    experimental::stts::globals::s_pool->addDependencyToCurrentTask(handle_);
  }
  ~Task() noexcept {
    HIGAN_ASSERT(handle_.done(), "coroutine was destroyed by the creator coroutine before coroutine was completed.");
    assert(handle_.done());
    handle_.destroy();
  }

  void wait() noexcept
  {
    if (!handle_.done())
      experimental::stts::globals::s_pool->execute();
  }

/*
  bool is_ready() const {
    return handle_ && handle_.done();
  }
  explicit operator bool() const {
    return !handle_.done();
  }*/
  // unwrap() future<future<int>> -> future<int>
  // future then(lambda) -> attach function to be done after current Task.
  // is_ready() are you ready?
private:
  std::experimental::coroutine_handle<> handle_;
};

template<typename Func>
Task<void> runAsync(Func&& f)
{
  f();
  co_return;
}

}
}
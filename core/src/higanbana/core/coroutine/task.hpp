#pragma once
#include "higanbana/core/coroutine/coroutine_pool.hpp"
#include <experimental/coroutine>

namespace higanbana
{
namespace coro
{
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
struct noop_task {
  struct promise_type {
    noop_task get_return_object() noexcept {
      return { std::experimental::coroutine_handle<promise_type>::from_promise(*this) };
    }
    void unhandled_exception() noexcept {}
    void return_void() noexcept {}
    suspend_always initial_suspend() noexcept { return {}; }
    suspend_never final_suspend() noexcept { return {}; }
  };
  std::experimental::coroutine_handle<> coro;
};

std::experimental::coroutine_handle<> noop_coroutine() noexcept;
std::experimental::coroutine_handle<> noop_coroutine(experimental::Barrier dep) noexcept;

template<typename T>
class task {
public:
  struct promise_type {
    using coro_handle = std::experimental::coroutine_handle<promise_type>;
    __declspec(noinline) auto get_return_object() noexcept {
      return task(coro_handle::from_promise(*this));
    }
    constexpr suspend_always initial_suspend() noexcept {
      return {};
    }

    struct final_awaiter {
      constexpr bool await_ready() noexcept {
        return false;
      }
      std::experimental::coroutine_handle<> await_suspend(coro_handle h) noexcept {
        if (h.promise().m_continuation.address() != nullptr){
          return h.promise().m_continuation;
        }
        return noop_coroutine(std::move(h.promise().finalDependency));
      }
      void await_resume() noexcept {}
    };

    final_awaiter final_suspend() noexcept {
      return {};
    }
    void return_value(T value) noexcept {m_value = std::move(value);}
    void unhandled_exception() noexcept {
      std::terminate();
    }
    T m_value;
    bool async = false;
    experimental::BarrierObserver bar;
    experimental::Barrier finalDependency;
    std::experimental::coroutine_handle<> m_continuation;
  };
  using coro_handle = std::experimental::coroutine_handle<promise_type>;
  task(coro_handle handle) noexcept : handle_(handle)
  {
    assert(handle);
    handle_.promise().finalDependency = experimental::Barrier();
    handle_.promise().async = true;
    handle_.promise().bar = experimental::globals::s_pool->task({}, [handlePtr = handle_.address()](size_t) mutable {
      auto handle = coro_handle::from_address(handlePtr); 
      handle.resume();
    });
  }
  task(task& other) noexcept {
    handle_ = other.handle_;
  };
  task(task&& other) noexcept {
    if (other.handle_)
      handle_ = std::move(other.handle_);
    assert(handle_);
    other.handle_ = nullptr;
  }
  task& operator=(task& other) noexcept {
    handle_ = other.handle_;
    return *this;
  };
  task& operator=(task&& other) noexcept {
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
  std::experimental::coroutine_handle<> await_suspend(coro_handle handle) noexcept {
    if(handle_.promise().async)
    {
      experimental::BarrierObserver obs;
      auto finalDep = handle_.promise().finalDependency;
      if (finalDep)
        obs = experimental::BarrierObserver(finalDep);
      experimental::Barrier temp;
      experimental::BarrierObserver tobs(temp);
      handle.promise().async = true;
      HIGAN_ASSERT(experimental::globals::s_pool, "thread pool not available, error");
      handle.promise().bar = experimental::globals::s_pool->task({std::move(obs), handle.promise().bar, tobs}, [handlePtr = handle.address()](size_t) mutable {
        auto handle = coro_handle::from_address(handlePtr);
        assert(!handle.done());
        handle.resume();
      });
      return noop_coroutine(temp);
    }
    handle_.promise().m_continuation = handle;
    return handle_;
  }
  ~task() noexcept {
    if (handle_ && handle_.done()) {
      assert(handle_.done());
      handle_.destroy();
    }
  }

  T get() noexcept
  {
    auto obs = experimental::BarrierObserver(handle_.promise().finalDependency);
    bool wasIAsync = handle_.promise().async;
    while(!handle_.done()){
      if (handle_.promise().async)
        experimental::globals::s_pool->helpTasksUntilBarrierComplete(obs);
      else {
        handle_.resume();
        if (!wasIAsync && handle_.promise().async)
        {
          wasIAsync = true;
          printf("Am async now! \n");
        }
      }
    }
    // we are exiting here too fast, somebody with stack left and our destruction kills it.
    assert(experimental::BarrierObserver(handle_.promise().finalDependency).done());
    assert(handle_.done());
    return handle_.promise().m_value; 
  }
  // unwrap() future<future<int>> -> future<int>
  // future then(lambda) -> attach function to be done after current task.
  // is_ready() are you ready?
private:
  std::experimental::coroutine_handle<promise_type> handle_;
};
}
}
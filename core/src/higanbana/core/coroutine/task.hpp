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

extern thread_local bool thread_first_seen_coroutine;
extern thread_local bool thread_inside_coroutine;
extern thread_local int thread_coroutine_depth;
template<typename T>
class Task {
public:
  struct promise_type {
    using coro_handle = std::experimental::coroutine_handle<promise_type>;
    __declspec(noinline) auto get_return_object() noexcept {
      return Task(coro_handle::from_promise(*this));
    }
    constexpr suspend_always initial_suspend() noexcept {
      return {};
    }

    struct final_awaiter {
      constexpr bool await_ready() noexcept {
        return false;
      }
      template<typename U>
      std::experimental::coroutine_handle<> await_suspend(U h) noexcept {
        if (h.promise().m_continuation.address() != nullptr){
          thread_first_seen_coroutine = false;
          thread_coroutine_depth++;
          return h.promise().m_continuation;
        }
        thread_coroutine_depth = 0;
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
  Task(coro_handle handle) noexcept : handle_(handle)
  {
    assert(handle);
    handle_.promise().finalDependency = experimental::Barrier();
    if (!thread_inside_coroutine || thread_first_seen_coroutine || thread_coroutine_depth > 2) {
      handle_.promise().async = true;
      handle_.promise().bar = experimental::globals::s_pool->task({}, [handlePtr = handle_.address()](size_t) mutable {
        auto handle = coro_handle::from_address(handlePtr);
        thread_first_seen_coroutine = false;
        thread_inside_coroutine = true;
        thread_coroutine_depth = 0; 
        handle.resume();
        thread_inside_coroutine = false;
      });
    }
    thread_first_seen_coroutine = true;
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
  template<typename HopefullyCoroHandleWithMyPromise>
  std::experimental::coroutine_handle<> await_suspend(HopefullyCoroHandleWithMyPromise handle) noexcept {
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
        thread_first_seen_coroutine = false; 
        thread_inside_coroutine = true;
        thread_coroutine_depth = 0;
        handle.resume();
        thread_inside_coroutine = false;
      });
      thread_coroutine_depth = 0;
      return noop_coroutine(temp);
    }
    handle_.promise().m_continuation = handle;
    thread_first_seen_coroutine = false;
    thread_coroutine_depth++;
    return handle_;
  }
  ~Task() noexcept {
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
          //printf("Am async now! \n");
        }
      }
    }
    // we are exiting here too fast, somebody with stack left and our destruction kills it.
    assert(experimental::BarrierObserver(handle_.promise().finalDependency).done());
    assert(handle_.done());
    return handle_.promise().m_value; 
  }
  bool is_ready() const {
    return handle_ && handle_.done();
  }
  explicit operator bool() const {
    return handle_.address() != nullptr;
  }
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
    constexpr suspend_always initial_suspend() noexcept {
      return {};
    }

    struct final_awaiter {
      constexpr bool await_ready() noexcept {
        return false;
      }

      template<typename U>
      std::experimental::coroutine_handle<> await_suspend(U h) noexcept {
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
    void return_void() noexcept {}
    void unhandled_exception() noexcept {
      std::terminate();
    }
    bool async = false;
    experimental::BarrierObserver bar;
    experimental::Barrier finalDependency;
    std::experimental::coroutine_handle<> m_continuation;
  };
  using coro_handle = std::experimental::coroutine_handle<promise_type>;
  Task(coro_handle handle) noexcept : handle_(handle)
  {
    assert(handle);
    handle_.promise().finalDependency = experimental::Barrier();
    handle_.promise().async = true;
    handle_.promise().bar = experimental::globals::s_pool->task({}, [handlePtr = handle_.address()](size_t) mutable {
      auto handle = coro_handle::from_address(handlePtr); 
      handle.resume();
    });
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
  template<typename HopefullyCoroHandleWithMyPromise>
  std::experimental::coroutine_handle<> await_suspend(HopefullyCoroHandleWithMyPromise handle) noexcept {
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
  ~Task() noexcept {
    if (handle_ && handle_.done()) {
      assert(handle_.done());
      handle_.destroy();
    }
  }

  void wait() noexcept
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
          //printf("Am async now! \n");
        }
      }
    }
    // we are exiting here too fast, somebody with stack left and our destruction kills it.
    assert(experimental::BarrierObserver(handle_.promise().finalDependency).done());
    assert(handle_.done());
  }

  bool is_ready() const {
    return handle_ && handle_.done();
  }
  explicit operator bool() const {
    return !handle_.done();
  }
  // unwrap() future<future<int>> -> future<int>
  // future then(lambda) -> attach function to be done after current Task.
  // is_ready() are you ready?
private:
  std::experimental::coroutine_handle<promise_type> handle_;
};

template<typename Func>
Task<void> runAsync(Func&& f)
{
  f();
  co_return;
}

}
}
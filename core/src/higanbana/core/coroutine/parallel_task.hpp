#pragma once
#include "higanbana/core/coroutine/coroutine_pool.hpp"
#include "higanbana/core/coroutine/task.hpp"
#include <experimental/coroutine>

namespace higanbana
{
namespace coro
{
namespace lol
{
  extern thread_local experimental::BarrierObserver __barrierToWait;
}

class ParallelTask {
public:
  struct promise_type {
    ParallelTask get_return_object() noexcept {
      return ParallelTask(std::experimental::coroutine_handle<promise_type>::from_promise(*this));
    }
    void unhandled_exception() noexcept {}
    void return_void() noexcept {}
    suspend_always initial_suspend() noexcept { return {}; }
    suspend_never final_suspend() noexcept { return {}; }
  };
  ParallelTask(std::experimental::coroutine_handle<> handle) noexcept
  {
    handle_ = handle;
    m_bar = lol::__barrierToWait;
  }
  ParallelTask(ParallelTask& other) noexcept {
    m_bar = other.m_bar;
  };
  ParallelTask(ParallelTask&& other) noexcept {
    m_bar = std::move(other.m_bar);
  }
  ParallelTask& operator=(ParallelTask& other) noexcept {
    m_bar = other.m_bar;
    return *this;
  };
  ParallelTask& operator=(ParallelTask&& other) noexcept {
    m_bar = std::move(other.m_bar);
    return *this;
  }
  void await_resume() noexcept {
  }
  bool await_ready() noexcept {
    return m_bar.done();
  }

  // enemy coroutine needs this coroutines result, therefore we compute it.
  template<typename HopefullyCoroHandleWithMyPromise>
  std::experimental::coroutine_handle<> await_suspend(HopefullyCoroHandleWithMyPromise handle) noexcept {
    experimental::Barrier temp;
    experimental::BarrierObserver tobs(temp);
    handle.promise().async = true;
    HIGAN_ASSERT(experimental::globals::s_pool, "thread pool not available, error");
    auto ownBarrier = m_bar;
    handle.promise().bar = experimental::globals::s_pool->task({ownBarrier, tobs}, [handlePtr = handle.address()](size_t) mutable {
      auto handle = std::experimental::coroutine_handle<>::from_address(handlePtr);
      assert(!handle.done());
      handle.resume();
    });
    return noop_coroutine(temp);
  }
  ~ParallelTask() noexcept {
  }

  void wait() noexcept
  {
    experimental::globals::s_pool->helpTasksUntilBarrierComplete(m_bar);
  }
  bool is_ready() const {
    return m_bar.done();
  }
  // unwrap() future<future<int>> -> future<int>
  // future then(lambda) -> attach function to be done after current Task.
  // is_ready() are you ready?
private:
  std::experimental::coroutine_handle<> handle_;
  experimental::BarrierObserver m_bar;
};

namespace __internal
{
ParallelTask createEmptyStack();
}

template<int ppt, typename Func>
ParallelTask parallelFor(size_t startIndex, size_t iterations, Func&& f)
{
  HIGAN_ASSERT(iterations != 0, "iterations shouldn't be 0, don't call if you didn't mean!");
  if (iterations > 0)
    lol::__barrierToWait = experimental::globals::s_pool->internalAddTask<ppt>({}, startIndex, iterations, std::forward<decltype(f)>(f));
  return __internal::createEmptyStack();
}
}
}
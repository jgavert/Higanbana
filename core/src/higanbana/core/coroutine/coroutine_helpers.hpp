#pragma once
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
}
}
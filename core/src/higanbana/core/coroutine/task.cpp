#include "higanbana/core/coroutine/task.hpp"

namespace higanbana
{
namespace coro
{
std::experimental::coroutine_handle<> noop_coroutine() noexcept{
  return []() -> noop_task {
    co_return;
  }().coro;
}
std::experimental::coroutine_handle<> noop_coroutine(experimental::Barrier dep) noexcept {
  return [b = std::move(dep)]() mutable -> noop_task {
    b.kill();
    co_return;
  }().coro;
}
}
}
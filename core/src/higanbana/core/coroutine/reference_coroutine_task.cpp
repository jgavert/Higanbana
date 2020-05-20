#include "higanbana/core/coroutine/reference_coroutine_task.hpp"

namespace higanbana
{
namespace experimental
{
namespace reference
{
namespace globals 
{
  std::unique_ptr<CoroutineExecutor> s_pool;
  void createExecutor() {
    if (!s_pool) s_pool = std::make_unique<CoroutineExecutor>();
  }
}
}
}
}
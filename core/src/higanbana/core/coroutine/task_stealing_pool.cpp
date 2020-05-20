#include "higanbana/core/coroutine/task_stealing_pool.hpp"

namespace higanbana
{
namespace taskstealer 
{
namespace locals 
{
thread_local bool thread_from_pool = false;
thread_local int thread_id = -1;
}
namespace globals 
{
  std::unique_ptr<TaskStealingPool> s_stealPool;
  void createTaskStealingPool() {
    if (!s_stealPool) s_stealPool = std::make_unique<TaskStealingPool>();
  }
}
}
}
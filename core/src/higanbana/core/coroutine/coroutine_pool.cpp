#include "higanbana/core/coroutine/coroutine_pool.hpp"

namespace higanbana
{
namespace experimental
{
namespace internal
{
thread_local bool thread_from_pool = false;
thread_local int thread_id = -1;
}
namespace globals 
{
  std::unique_ptr<LBSPool> s_pool;
  void createGlobalLBSPool() {
    if (!s_pool) s_pool = std::make_unique<LBSPool>();
  }
}
}
}
#include "higanbana/core/coroutine/coroutine_pool_st.hpp"

namespace higanbana
{
namespace experimental
{
namespace stts
{
namespace v2
{
thread_local bool thread_from_pool = false;
thread_local int thread_id = -1;
}
namespace globals 
{
  std::unique_ptr<SingleThreadPool> s_pool;
  void createSingleThreadPool() {
    if (!s_pool) s_pool = std::make_unique<SingleThreadPool>();
  }
}
}
}
}
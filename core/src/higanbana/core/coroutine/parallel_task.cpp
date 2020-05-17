#include "higanbana/core/coroutine/parallel_task.hpp"

namespace higanbana
{
namespace coro
{
namespace __internal
{
ParallelTask createEmptyStack(){
  co_return; // the body is never computer, purpose is giving compiler a expected function body.
}
}
}
}
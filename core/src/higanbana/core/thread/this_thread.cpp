#include "higanbana/core/thread/this_thread.hpp"

namespace higanbana
{
namespace thread
{
namespace this_thread
{
namespace
{
  thread_local int myIndex;
}
int id()
{
  return myIndex;
}
}
}
}
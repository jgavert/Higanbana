#include <higanbana/core/platform/EntryPoint.hpp>
#include "windowMain.hpp"

higanbana::coro::task<int> EntryPoint::main()
{	
  mainWindow(m_params);

  co_return 0;
}
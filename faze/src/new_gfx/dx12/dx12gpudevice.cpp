#if defined(PLATFORM_WINDOWS)
#include "dx12resources.hpp"

#include "core/src/global_debug.hpp"

namespace faze
{
  namespace backend
  {
    DX12Device::DX12Device(
      GpuInfo info)
      : m_info(info)
    {
  
    }
  }
}
#endif
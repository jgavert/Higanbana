#if defined(PLATFORM_WINDOWS)
#include "dx12resources.hpp"

#include "core/src/global_debug.hpp"

namespace faze
{
  namespace backend
  {
    DX12Device::DX12Device(GpuInfo info, ID3D12Device* device)
      : m_info(info)
      , m_device(device)
    {
  
    }
  }
}
#endif
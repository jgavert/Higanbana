#pragma once
#include "ComPtr.hpp"
#include <d3d12.h>

class EvolRes
{
private:
  friend class GpuDevice;
  friend class GfxCommandList;

  ComPtr<ID3D12Resource> m_res;
public:

};

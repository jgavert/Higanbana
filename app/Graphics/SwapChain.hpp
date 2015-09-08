#pragma once
#include "ComPtr.hpp"
#include <DXGI1_4.h>
#include <d3d12.h>

class SwapChain
{
private:
  friend class GpuDevice;
  IDXGISwapChain3* m_SwapChain;

  SwapChain(IDXGISwapChain3* SwapChain);
public:
  SwapChain():m_SwapChain(nullptr)
  {
  };
};
#pragma once
#include "ComPtr.hpp"
#include "Texture.hpp"

#include <vector>
#include <DXGI1_4.h>
#include <d3d12.h>

class SwapChain
{
private:
  friend class GpuDevice;

  std::vector<TextureRTV> m_resources;
  ComPtr<IDXGISwapChain3> m_SwapChain;
  
  SwapChain(ComPtr<IDXGISwapChain3> SwapChain) :
    m_SwapChain(std::move(SwapChain))
  {}

  SwapChain(ComPtr<IDXGISwapChain3> SwapChain, std::vector<TextureRTV> resources) :
    m_SwapChain(std::move(SwapChain)), m_resources(resources)
  {}

  SwapChain():m_SwapChain(nullptr)
  {
  }
public:

  ~SwapChain()
  {
    //m_SwapChain->SetFullscreenState(FALSE, nullptr);
  }

  TextureRTV& operator[](size_t i)
  {
    return m_resources.at(i);
  }

  size_t rtvCount()
  {
    return m_resources.size();
  }

  bool valid()
  {
    bool arr = m_SwapChain.get() != nullptr;
    for (auto&& it : m_resources)
    {
      if (it.textureRTV() == nullptr)
        return false;
    }
    return arr;
  }

  IDXGISwapChain3* operator->() const throw()
  {
    return m_SwapChain.operator->();
  }

};

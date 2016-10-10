#pragma once

#include "vk_specifics/VulkanSwapchain.hpp"

#include "Texture.hpp"

enum class PresentMode
{
	Immediate,
	Mailbox,
	Fifo,
	FifoRelaxed
};

#include <vector>

class Swapchain
{
private:
  friend class GpuDevice;

  SwapchainImpl m_SwapChain;
  std::vector<TextureRTV> m_resources;


public:
  Swapchain() {}
  Swapchain(SwapchainImpl swapchain)
    : m_SwapChain(swapchain)
  {}

  Swapchain(SwapchainImpl swapchain, std::vector<TextureRTV> resources)
    : m_SwapChain(swapchain)
    , m_resources(resources)
  {
  }

  ~Swapchain()
  {
  }

  TextureRTV& operator[](size_t )
  {
    return m_resources.at(0);
  }

  size_t rtvCount()
  {
    return m_resources.size();
  }

  unsigned GetCurrentBackBufferIndex()
  {
    return 0;
  }

  void present(unsigned, unsigned)
  {

  }

  bool valid()
  {
    return true;
  }
};

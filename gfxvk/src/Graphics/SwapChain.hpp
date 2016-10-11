#pragma once

#include "vk_specifics/VulkanSwapchain.hpp"
#include "vk_specifics/VulkanSemaphore.hpp"

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

  SwapchainImpl m_swapchain;
  std::vector<TextureRTV> m_resources;
  SemaphoreImpl m_pre;
  SemaphoreImpl m_post;
  int m_currentIndex;
public:
  Swapchain() {}
  Swapchain(SwapchainImpl swapchain)
    : m_swapchain(swapchain)
  {}

  Swapchain(SwapchainImpl swapchain, std::vector<TextureRTV> resources, SemaphoreImpl waitImage, SemaphoreImpl renderDone)
    : m_swapchain(swapchain)
    , m_resources(resources)
	, m_pre(waitImage)
	, m_post(renderDone)
  {
  }

  ~Swapchain()
  {
  }

  TextureRTV& operator[](size_t i)
  {
    return m_resources.at(i);
  }

  size_t rtvCount()
  {
    return m_resources.size();
  }

  int lastAcquiredIndex()
  {
	  return m_currentIndex;
  }

  bool valid()
  {
    return true;
  }
};

#pragma once

#include "vk_specifics/VulkanSwapchain.hpp"
#include "vk_specifics/VulkanSemaphore.hpp"

#include "Texture.hpp"

enum class PresentMode
{
  Unknown,
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
  ResourceDescriptor m_descriptor;
  PresentMode m_mode;
  int m_currentIndex = 0;
public:
  Swapchain() {}
  Swapchain(SwapchainImpl swapchain)
    : m_swapchain(swapchain)
  {}

  Swapchain(SwapchainImpl swapchain, std::vector<TextureRTV> resources, SemaphoreImpl waitImage, SemaphoreImpl renderDone, ResourceDescriptor descriptor, PresentMode mode)
    : m_swapchain(swapchain)
    , m_resources(std::forward<decltype(resources)>(resources))
    , m_pre(waitImage)
    , m_post(renderDone)
    , m_descriptor(std::forward<decltype(descriptor)>(descriptor))
    , m_mode(mode)
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

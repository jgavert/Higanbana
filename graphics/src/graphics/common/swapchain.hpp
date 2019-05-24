#pragma once

#include "graphics/common/backend.hpp"
#include "graphics/common/resources.hpp"
#include "graphics/common/resource_descriptor.hpp"
#include "graphics/common/prototypes.hpp"

namespace faze
{
  class Swapchain : public backend::GpuDeviceChild
  {
    std::shared_ptr<backend::prototypes::SwapchainImpl> m_impl;
    vector<TextureRTV> m_backbuffers;

  public:
    Swapchain() = default;

    Swapchain(std::shared_ptr<backend::prototypes::SwapchainImpl> impl)
      : m_impl(impl)
    {
    }

    std::shared_ptr<backend::prototypes::SwapchainImpl> impl()
    {
      return m_impl;
    }

    void setBackbuffers(vector<TextureRTV> buffers)
    {
      m_backbuffers = buffers;
    }

    vector<TextureRTV>& buffers()
    {
      return m_backbuffers;
    }
  };
};
#pragma once

#include "higanbana/graphics/common/backend.hpp"
#include "higanbana/graphics/common/resources.hpp"
#include "higanbana/graphics/common/resource_descriptor.hpp"
#include "higanbana/graphics/common/prototypes.hpp"

namespace higanbana
{
  class Swapchain : public backend::GpuGroupChild
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

    bool outOfDate()
    {
      return m_impl->outOfDate();
    }
  };
};
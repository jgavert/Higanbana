#pragma once

#include "backend.hpp"
#include "resources.hpp"
#include "resource_descriptor.hpp"
#include "implementation.hpp"

namespace faze
{
  class Swapchain : public backend::GpuDeviceChild
  {
    std::shared_ptr<backend::prototypes::SwapchainImpl> m_impl;
    std::shared_ptr<int64_t> m_id;
    vector<Texture> m_backbuffers;

  public:
    Swapchain() = default;

    Swapchain(std::shared_ptr<backend::prototypes::SwapchainImpl> impl, std::shared_ptr<int64_t> id)
      : m_impl(impl)
      , m_id(id)
    {
    }

    std::shared_ptr<backend::prototypes::SwapchainImpl> impl()
    {
      return m_impl;
    }

    void setBackbuffers(vector<Texture> buffers)
    {
      m_backbuffers = buffers;
    }

    vector<Texture>& buffers()
    {
      return m_backbuffers;
    }
  };
};
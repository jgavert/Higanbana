#pragma once

#include "backend.hpp"
#include "resources.hpp"
#include "resource_descriptor.hpp"
#include "implementation.hpp"

namespace faze
{
  class Swapchain : public backend::GpuDeviceChild
  {
    std::shared_ptr<backend::prototypes::SwapchainImpl> impl;
    std::shared_ptr<int64_t> id;

  public:
    Swapchain() = default;

    Swapchain(std::shared_ptr<backend::prototypes::SwapchainImpl> impl, std::shared_ptr<int64_t> id)
      : impl(impl)
      , id(id)
    {
    }
  };
};
#pragma once

#include "prototypes.hpp"
#include <memory>

namespace faze
{
  class GraphicsSurface : public backend::GpuDeviceChild
  {
    std::shared_ptr<backend::prototypes::GraphicsSurfaceImpl> impl;
  public:
    GraphicsSurface() = default;

    GraphicsSurface(std::shared_ptr<backend::prototypes::GraphicsSurfaceImpl> impl)
      : impl(impl)
    {
    }
  };
}
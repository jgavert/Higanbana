#pragma once

#include "higanbana/graphics/common/prototypes.hpp"
#include <memory>

namespace higanbana
{
  class GraphicsSurface : public backend::GpuGroupChild
  {
    std::shared_ptr<backend::prototypes::GraphicsSurfaceImpl> impl;
  public:
    GraphicsSurface() = default;

    GraphicsSurface(std::shared_ptr<backend::prototypes::GraphicsSurfaceImpl> impl)
      : impl(impl)
    {
    }

    std::shared_ptr<backend::prototypes::GraphicsSurfaceImpl> native()
    {
      return impl;
    }
  };
}
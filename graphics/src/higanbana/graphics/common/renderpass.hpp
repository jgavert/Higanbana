#pragma once
#include "higanbana/graphics/common/handle.hpp"

#include <memory>

namespace higanbana
{
  class Renderpass
  {
    std::shared_ptr<ResourceHandle> m_renderpass;

  public:
    Renderpass() {}

    Renderpass(std::shared_ptr<ResourceHandle> native)
      : m_renderpass(native)
    {
    }

    ResourceHandle handle()
    {
      if (m_renderpass)
        return *m_renderpass;
      return ResourceHandle();
    }
  };
}
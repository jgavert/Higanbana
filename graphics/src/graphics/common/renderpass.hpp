#pragma once

#include <graphics/common/handle.hpp>

#include <memory>

namespace faze
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
      return InvalidResourceHandle;
    }
  };
}
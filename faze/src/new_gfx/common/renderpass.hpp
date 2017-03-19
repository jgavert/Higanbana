#pragma once

#include "faze/src/new_gfx/common/prototypes.hpp"

#include <memory>

namespace faze
{
  class Renderpass
  {
    std::shared_ptr<backend::prototypes::RenderpassImpl> m_renderpass;

  public:
    Renderpass(std::shared_ptr<backend::prototypes::RenderpassImpl> native)
      : m_renderpass(native)
    {
    }

    std::shared_ptr<backend::prototypes::RenderpassImpl> impl()
    {
      return m_renderpass;
    }
  };
}
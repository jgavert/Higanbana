#pragma once

#include "graphics/common/backend.hpp"
#include "graphics/common/resources.hpp"
#include "graphics/common/resource_descriptor.hpp"
#include "graphics/common/prototypes.hpp"

namespace faze
{
  class GpuSemaphore
  {
    std::shared_ptr<backend::SemaphoreImpl> m_impl;
  public:
    GpuSemaphore(std::shared_ptr<backend::SemaphoreImpl> sema)
      : m_impl(sema)
    {
    }

    std::shared_ptr<backend::SemaphoreImpl> native()
    {
      return m_impl;
    }
  };

  class GpuSharedSemaphore
  {
    GpuSemaphore m_primary;
    GpuSemaphore m_secondary;
  public:
    GpuSharedSemaphore(GpuSemaphore primary, GpuSemaphore secondary)
      : m_primary(primary)
      , m_secondary(secondary)
    {
    }

    GpuSemaphore& primary()
    {
      return m_primary;
    }

    GpuSemaphore& secondary()
    {
      return m_secondary;
    }
  };
}
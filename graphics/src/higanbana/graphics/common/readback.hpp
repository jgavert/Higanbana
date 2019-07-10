#pragma once
#include "higanbana/graphics/common/handle.hpp"

namespace higanbana
{
  class ReadbackData
  {
    std::shared_ptr<ResourceHandle> m_id; // only id for now
  public: 
    ReadbackData()
      : m_id(std::make_shared<ResourceHandle>())
    {
    }

    ReadbackData(std::shared_ptr<ResourceHandle> id)
      : m_id(id)
    {
    }

    ResourceHandle handle() const
    {
      if (m_id)
        return *m_id;
      return ResourceHandle();
    }
  };
}
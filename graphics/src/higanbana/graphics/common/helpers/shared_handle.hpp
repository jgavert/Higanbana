#pragma once

#include "higanbana/graphics/common/resources/graphics_api.hpp"
#include <higanbana/core/platform/definitions.hpp>

namespace higanbana
{
namespace backend
{
#if defined(HIGANBANA_PLATFORM_WINDOWS)
    struct SharedHandle
    {
      GraphicsApi api;
      void* handle;
      size_t heapsize;
    };
#else
    struct SharedHandle
    {
      int woot;
    };
#endif
}
}
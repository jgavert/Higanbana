#pragma once
#include "higanbana/graphics/common/pipeline.hpp"
#include "higanbana/graphics/common/renderpass.hpp"
#include "higanbana/core/datastructures/enum_array.hpp"

namespace higanbana
{
  template<typename... Enums>
  using GraphicsPipelines = EnumArray<GraphicsPipeline, Enums...>;
  template<typename... Enums>
  using Renderpasses = EnumArray<Renderpass, Enums...>;
}
#pragma once

#include "higanbana/graphics/common/backend.hpp"
#include "higanbana/graphics/common/resources.hpp"
#include "higanbana/graphics/common/pipeline.hpp"
#include "higanbana/graphics/common/handle.hpp"
#include "higanbana/graphics/common/resources/shader_arguments.hpp"

namespace higanbana
{
  class ShaderArgumentsBinding 
  {
    vector<ShaderArgumentsLayout> m_layouts;
    vector<ShaderArguments> m_arguments;
    vector<uint8_t> m_constants;

  public:
    ShaderArgumentsBinding(GraphicsPipeline pipeline)
      : m_layouts(pipeline.descriptor.desc.layout.m_sets)
      , m_arguments(m_layouts.size())
    {

    }

    ShaderArgumentsBinding(ComputePipeline pipeline)
      : m_layouts(pipeline.descriptor.layout.m_sets)
      , m_arguments(m_layouts.size())
    {

    }

    MemView<uint8_t> bConstants()
    {
      return MemView<uint8_t>(m_constants);
    }

    MemView<ShaderArguments> bShaderArguments()
    {
      return MemView<ShaderArguments>(m_arguments);
    }

    template <typename T>
    void constants(T consts)
    {
      m_constants.resize(sizeof(T));
      memcpy(m_constants.data(), &consts, sizeof(T));
    }

    void bind(int set, ShaderArguments args)
    {
      HIGAN_ASSERT(set < m_layouts.size() && set >= 0, "Invalid set");
      HIGAN_ASSERT(args.handle().id != ResourceHandle::InvalidId, "Invalid ShaderArguments given.");
      m_arguments[set] = args;
    }
  };
}

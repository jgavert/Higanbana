#pragma once
#include "higanbana/graphics/desc/formats.hpp"
#include "higanbana/graphics/desc/shader_input_descriptor.hpp"
#include <higanbana/core/math/math.hpp>
#include <string>
#include <array>

namespace higanbana
{
  class ShaderArgumentsLayoutDescriptor
  {
  public:
    ShaderInputDescriptor layout;

    ShaderArgumentsLayoutDescriptor(ShaderInputDescriptor)
    {
    }

    ComputePipelineDescriptor& setLayout(ShaderInputDescriptor val)
    {
      layout = val;
      return *this;
    }
  };
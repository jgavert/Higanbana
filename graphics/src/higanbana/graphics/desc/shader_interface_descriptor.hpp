#pragma once

#include "higanbana/graphics/common/resources/shader_arguments.hpp"
#include <string>

namespace higanbana
{
  class ShaderInterfaceDescriptor 
  {
  public:
    std::string constantStructBody;
    vector<ShaderArgumentsLayout> m_sets;
    size_t constantsSizeOf;

    ShaderInterfaceDescriptor()
      : constantsSizeOf(0)
    {
      m_sets.resize(3);
    }
  public:

    template<typename Strct>
    ShaderInterfaceDescriptor& constants()
    {
      constantsSizeOf = sizeof(Strct);
      constantStructBody = Strct::structMembersAsString;
      return *this;
    }

    ShaderInterfaceDescriptor& shaderArguments(unsigned setNumber, ShaderArgumentsLayout layout)
    {
      HIGAN_ASSERT(setNumber <= 2, "Only 3 usable sets, please manage.");
      m_sets[setNumber] = layout;
      return *this;
    }

    std::string createInterface();
  };
}
#pragma once
#include "higanbana/graphics/definitions.hpp"
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
      m_sets.resize(HIGANBANA_USABLE_SHADER_ARGUMENT_SETS);
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
      HIGAN_ASSERT(setNumber < HIGANBANA_USABLE_SHADER_ARGUMENT_SETS, "Only %d usable sets, please manage.", HIGANBANA_USABLE_SHADER_ARGUMENT_SETS);
      m_sets[setNumber] = layout;
      return *this;
    }

    std::string createInterface();
  };
}
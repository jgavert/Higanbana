#pragma once
#include "higanbana/graphics/definitions.hpp"
#include "higanbana/graphics/common/resources/shader_arguments.hpp"
#include <string>

namespace higanbana
{
  class PipelineInterfaceDescriptor 
  {
  public:
    vector<std::string> m_structs;
    std::string constantStructBody;
    vector<ShaderArgumentsLayout> m_sets;
    size_t constantsSizeOf;

    PipelineInterfaceDescriptor()
      : constantsSizeOf(0)
    {
    }
  public:

    template<typename Strct>
    PipelineInterfaceDescriptor& structDecl() {
      auto sd = "struct " + std::string(Strct::structNameAsString) + " { " + std::string(Strct::structMembersAsString) + " };";
      m_structs.push_back(sd);
      return *this;
    }

    template<typename Strct>
    PipelineInterfaceDescriptor& constants()
    {
      constantsSizeOf = sizeof(Strct);
      constantStructBody = Strct::structMembersAsString;
      return *this;
    }

    PipelineInterfaceDescriptor& shaderArguments(unsigned setNumber, ShaderArgumentsLayout layout)
    {
      HIGAN_ASSERT(setNumber < HIGANBANA_USABLE_SHADER_ARGUMENT_SETS, "Only %d usable sets, please manage.", HIGANBANA_USABLE_SHADER_ARGUMENT_SETS);
      if (m_sets.size() < HIGANBANA_USABLE_SHADER_ARGUMENT_SETS)
      {
        m_sets.resize(setNumber+1);
      }
      m_sets[setNumber] = layout;
      return *this;
    }

    std::string createInterface();
  };
}
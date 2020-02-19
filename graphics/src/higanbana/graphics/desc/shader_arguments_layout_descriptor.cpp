#include "higanbana/graphics/desc/shader_arguments_layout_descriptor.hpp"

namespace higanbana
{
  vector<ShaderResource> ShaderArgumentsLayoutDescriptor::getResources()
  {
    return sortedResources;
  }

  vector<std::pair<size_t, std::string>> ShaderArgumentsLayoutDescriptor::structDeclarations()
  {
    return structDecls;
  }
}
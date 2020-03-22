#pragma once

#include <higanbana/core/datastructures/proxy.hpp>
#include "higanbana/graphics/desc/pipeline_interface_descriptor.hpp"

namespace higanbana {
namespace backend {
enum class ShaderType {
  Unknown,
  Compute,
  Vertex,
  Pixel,
  Geometry,
  Hull,           // TessControl
  Domain,         // TessEvaluation
  Amplification,  // Task shader
  Mesh,           // Mesh shader
  Raytracing      // encompasses ... lots of stuff
};

enum class ShaderBinaryType { SPIRV, DXIL };

struct ShaderCreateInfo {
  struct Descriptor {
    std::string              shaderName = "";
    ShaderType               type = ShaderType::Compute;
    std::vector<std::string> definitions = {};
    uint3                    tgs = uint3(1, 1, 1);
    std::string              rootSignature = "";
    std::string              interfaceDeclaration = "";
    bool                     forceCompile = false;
  } desc;

  ShaderCreateInfo(std::string shaderName, ShaderType type, PipelineInterfaceDescriptor shaderInterface) {
    desc.shaderName = shaderName;
    desc.type = type;
    desc.interfaceDeclaration = shaderInterface.createInterface();
  }
  ShaderCreateInfo& compile() {
    desc.forceCompile = true;
    return *this;
  }
  ShaderCreateInfo& setDefinitions(std::vector<std::string> value) {
    desc.definitions = value;
    return *this;
  }
  ShaderCreateInfo& setComputeGroups(uint3 value) {
    desc.tgs = value;
    return *this;
  }
};
}  // namespace backend
}  // namespace higanbana
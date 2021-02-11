#include "higanbana/graphics/desc/shader_input_descriptor.hpp"

namespace higanbana
{
  const char* ShaderResourceTypeStr[] =
  {
    "Buffer",
    "ByteAddressBuffer",
    "StructuredBuffer",
    "Texture1D",
    "Texture1DArray",
    "Texture2D",
    "Texture2DArray",
    "Texture3D",
    "TextureCube",
    "TextureCubeArray",
    "RaytracingAccelerationStructure",
  };

  const char* toString(ShaderResourceType type)
  {
    return ShaderResourceTypeStr[static_cast<int>(type)];
  }
}
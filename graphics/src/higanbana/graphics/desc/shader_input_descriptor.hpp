#pragma once
#include <string>
#include <higanbana/core/datastructures/proxy.hpp>
#include <string_view>

namespace higanbana
{
  enum class ShaderResourceType
  {
    Buffer,
    ByteAddressBuffer,
    StructuredBuffer,
    Texture1D,
    Texture1DArray,
    Texture2D,
    Texture2DArray,
    Texture3D,
    TextureCube,
    TextureCubeArray,
    RaytracingAccelerationStructure
  };

  enum class ShaderElementType
  {
    Unknown,
    FloatingPoint,
    Unsigned,
    Integer
  };

  const char* toString(ShaderResourceType type);

  struct ShaderResource
  {
    ShaderResourceType type;
    std::string templateParameter;
    std::string name;
    ShaderElementType elementType = ShaderElementType::Unknown;
    bool readonly;
    bool bindless;
    int bindlessCountWorstCase;
    ShaderResource()
    {}
    ShaderResource(ShaderResourceType type, std::string templateParameter, std::string name, bool readonly, bool bindless = false, int descriptorCount = -1)
    : type(type)
    , templateParameter(templateParameter)
    , name(name)
    , readonly(readonly)
    , bindless(bindless)
    , bindlessCountWorstCase(descriptorCount)
    {
      std::string_view templ{templateParameter};
      std::string_view maybeFloat = templ.substr(0, std::min(size_t(5), templ.size()));
      std::string_view maybeInt = templ.substr(0, std::min(size_t(3), templ.size()));
      std::string_view maybeUint = templ.substr(0, std::min(size_t(4), templ.size()));
      if (maybeFloat.compare("float") == 0)
        elementType = ShaderElementType::FloatingPoint;
      if (maybeInt.compare("int") == 0)
        elementType = ShaderElementType::Integer;
      if (maybeUint.compare("uint") == 0)
        elementType = ShaderElementType::Unsigned;
    }
  };
};


#define STRINGIFY(a) #a
#define SHADER_STRUCT(name, args) \
struct name \
{ \
  args \
  static constexpr const char* structAsString = "struct " STRINGIFY(name) "{ " STRINGIFY(args) " };"; \
  static constexpr const char* structNameAsString = STRINGIFY(name); \
  static constexpr const char* structMembersAsString = STRINGIFY(args); \
};
/*  
STRUCT_DECL(kewl,
    int a;
    int b;
    int c;
);

int main()
{
    std::string interface = create_interface(ShaderInputDescriptor()
        .constants<kewl>()
        .readOnly(ShaderResourceType::ByteAddressBuffer, "test")
        .readOnly<kewl>(ShaderResourceType::Buffer, "test2")
        .readWrite(ShaderResourceType::Buffer, "float4", "testOut"));
    std::cout << interface << std::endl;
}
*/

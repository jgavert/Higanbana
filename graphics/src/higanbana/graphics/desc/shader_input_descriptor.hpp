#pragma once
#include <string>
#include <higanbana/core/datastructures/proxy.hpp>

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
  };

  const char* toString(ShaderResourceType type);

  struct ShaderResource
  {
    ShaderResourceType type;
    std::string templateParameter;
    std::string name;
    bool readonly;
    bool bindless;
    ShaderResource()
    {}
    ShaderResource(ShaderResourceType type, std::string templateParameter, std::string name, bool readonly, bool bindless = false)
    : type(type)
    , templateParameter(templateParameter)
    , name(name)
    , readonly(readonly)
    , bindless(bindless)
    {

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

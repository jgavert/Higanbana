#pragma once
#include <string>
#include <higanbana/core/datastructures/vector.hpp>
#include <higanbana/core/math/math.hpp>
#include <string_view>

#define STRINGIFY(a) #a
#define SHADER_STRUCT(name, args) \
struct name \
{ \
  args \
  static constexpr const char* structAsString = "struct " STRINGIFY(name) "{ " STRINGIFY(args) " };"; \
  static constexpr const char* structNameAsString = STRINGIFY(name); \
  static constexpr const char* structMembersAsString = STRINGIFY(args); \
};

namespace higanbana
{
  namespace Indirect
  {
    SHADER_STRUCT(DrawParams,
      uint VertexCountPerInstance;
      uint InstanceCount;
      uint StartVertexLocation;
      uint StartInstanceLocation;
    );

    SHADER_STRUCT(DrawIndexedParams,
      uint IndexCountPerInstance;
      uint InstanceCount;
      uint StartIndexLocation;
      int  BaseVertexLocation;
      uint StartInstanceLocation;
    );

    SHADER_STRUCT(DispatchParams,
      uint ThreadGroupCountX;
      uint ThreadGroupCountY;
      uint ThreadGroupCountZ;
    );

    SHADER_STRUCT(DispatchRaysParams,
      uint64_t RayGenerationShaderRecordStartAddress;
      uint64_t RayGenerationShaderRecordSizeInBytes;
      uint64_t MissShaderTableStartAddress;
      uint64_t MissShaderTableSizeInBytes;
      uint64_t MissShaderTableStrideInBytes;
      uint64_t HitGroupTableStartAddress;
      uint64_t HitGroupTableSizeInBytes;
      uint64_t HitGroupTableStrideInBytes;
      uint64_t CallableShaderTableStartAddress;
      uint64_t CallableShaderTableSizeInBytes;
      uint64_t CallableShaderTableStrideInBytes;
      uint Width;
      uint Height;
      uint Depth;
    );
  }
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



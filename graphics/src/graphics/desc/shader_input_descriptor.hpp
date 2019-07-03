#pragma once
#include <string>
#include <core/datastructures/proxy.hpp>

namespace faze
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
    ShaderResource()
    {}
    ShaderResource(ShaderResourceType type, std::string templateParameter, std::string name, bool readonly)
    : type(type)
    , templateParameter(templateParameter)
    , name(name)
    , readonly(readonly)
    {

    }
  };

  class ShaderInputDescriptor
  {
  public:
    std::string constantStructBody;
    vector<std::string> structDecls;
    size_t constantsSizeOf;
    vector<ShaderResource> sortedResources;

    ShaderInputDescriptor()
    {
      constantsSizeOf = 0;
    }
  private:
    int calcOrder(ShaderResourceType type, bool ro)
    {
      int val = static_cast<int>(type);
      constexpr int ImageBorder = static_cast<int>(ShaderResourceType::Texture1D);
      if (val < ImageBorder)
      {
        // buffer
        val = val * 2;
        if (!ro)
        {
          val += 1;
        }
      }
      else
      {
        // image
        val = val + ImageBorder;
        if (!ro)
        {
          val += 10;
        }
      }
      return val;
    }
    void insertSort(const ShaderResource& res)
    {
      #if 0
      auto findVal = calcOrder(res.type, res.readonly);
      auto iter = desc.sortedResources.begin();
      auto found = iter;
      for (; iter != desc.sortedResources.end(); iter++)
      {
        auto curVal = calcOrder(iter->type, iter->readonly);
        if (curVal > findVal)
        {
          break;
        }
      }
      
      desc.sortedResources.insert(iter, res);
      #endif
      sortedResources.push_back(res);
    }
  public:

    template<typename Strct>
    ShaderInputDescriptor& constants()
    {
      constantsSizeOf = sizeof(Strct);
      constantStructBody = Strct::structMembersAsString;
      return *this;
    }

    template <typename Strct>
    ShaderInputDescriptor& readOnly(ShaderResourceType type, std::string name)
    {
      auto sd = "struct " + std::string(Strct::structNameAsString) + " { " + std::string(Strct::structMembersAsString) + " };";

      auto res = ShaderResource(type, Strct::structNameAsString, name, true);
      insertSort(res);
      structDecls.emplace_back(sd);
      return *this;
    }

    ShaderInputDescriptor& readOnly(ShaderResourceType type, std::string name)
    {
      auto res = ShaderResource(type, "", name, true);
      insertSort(res);
      return *this;
    }
    ShaderInputDescriptor& readOnly(ShaderResourceType type, std::string templateParameter, std::string name)
    {
      auto res = ShaderResource(type, templateParameter, name, true);
      insertSort(res);
      return *this;
    }

      template <typename Strct>
    ShaderInputDescriptor& readWrite(ShaderResourceType type, std::string name)
    {
      auto sd = "struct " + Strct::structNameAsString + " { " + Strct::structMembersAsString + " };";

      auto res = ShaderResource(type, Strct::structNameAsString, name, false);
      insertSort(res);
      structDecls.emplace_back(sd);
      return *this;
    }
    ShaderInputDescriptor& readWrite(ShaderResourceType type, std::string name)
    {
      auto res = ShaderResource(type, "", name, false);
      insertSort(res);
      return *this;
    }
    ShaderInputDescriptor& readWrite(ShaderResourceType type, std::string templateParameter, std::string name)
    {
      auto res = ShaderResource(type, templateParameter, name, false);
      insertSort(res);
      return *this;
    }
    std::string createInterface();
    vector<ShaderResource> getResources();
  };
}

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

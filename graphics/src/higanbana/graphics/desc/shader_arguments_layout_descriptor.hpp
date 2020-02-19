#pragma once
#include "higanbana/graphics/desc/shader_input_descriptor.hpp"
#include <higanbana/core/datastructures/proxy.hpp>
#include <higanbana/core/external/SpookyV2.hpp>
#include <string>

namespace higanbana
{
  class ShaderArgumentsLayoutDescriptor 
  {
  public:
    std::string constantStructBody;
    vector<std::pair<size_t, std::string>> structDecls;
    vector<ShaderResource> sortedResources;

    ShaderArgumentsLayoutDescriptor()
    {
    }

    bool hasStruct(size_t key)
    {
      for (auto&& p : structDecls)
      {
        if (p.first == key)
          return true;
      }
      return false;
    }
  private:
    void insertSort(const ShaderResource& res)
    {
      sortedResources.push_back(res);
    }
  public:

    template <typename Strct>
    ShaderArgumentsLayoutDescriptor& readOnly(ShaderResourceType type, std::string name)
    {
      auto sd = "struct " + std::string(Strct::structNameAsString) + " { " + std::string(Strct::structMembersAsString) + " };";
      auto key = HashMemory(sd.data(), sd.size());

      auto res = ShaderResource(type, Strct::structNameAsString, name, true);
      insertSort(res);
      if (!hasStruct(key))
        structDecls.emplace_back(std::make_pair(key, sd));
      return *this;
    }

    ShaderArgumentsLayoutDescriptor& readOnly(ShaderResourceType type, std::string name)
    {
      auto res = ShaderResource(type, "", name, true);
      insertSort(res);
      return *this;
    }
    ShaderArgumentsLayoutDescriptor& readOnly(ShaderResourceType type, std::string templateParameter, std::string name)
    {
      auto res = ShaderResource(type, templateParameter, name, true);
      insertSort(res);
      return *this;
    }

    template <typename Strct>
    ShaderArgumentsLayoutDescriptor& readWrite(ShaderResourceType type, std::string name)
    {
      auto sd = "struct " + std::string(Strct::structNameAsString) + " { " + std::string(Strct::structMembersAsString) + " };";
      auto key = HashMemory(sd.data(), sd.size());

      auto res = ShaderResource(type, Strct::structNameAsString, name, false);
      insertSort(res);
      if (!hasStruct(key))
        structDecls.emplace_back(std::make_pair(key, sd));
      return *this;
    }
    ShaderArgumentsLayoutDescriptor& readWrite(ShaderResourceType type, std::string name)
    {
      auto res = ShaderResource(type, "", name, false);
      insertSort(res);
      return *this;
    }
    ShaderArgumentsLayoutDescriptor& readWrite(ShaderResourceType type, std::string templateParameter, std::string name)
    {
      auto res = ShaderResource(type, templateParameter, name, false);
      insertSort(res);
      return *this;
    }
    vector<ShaderResource> getResources();
    vector<std::pair<size_t, std::string>> structDeclarations();
  };
}
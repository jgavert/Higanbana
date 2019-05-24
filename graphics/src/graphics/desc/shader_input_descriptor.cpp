#include "graphics/desc/shader_input_descriptor.hpp"

namespace faze
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
  };

  const char* toString(ShaderResourceType type)
  {
    return ShaderResourceTypeStr[static_cast<int>(type)];
  }

  vector<ShaderResource> ShaderInputDescriptor::getResources()
  {
    return desc.sortedResources;
  }

  std::string ShaderInputDescriptor::createInterface()
  {
    std::string lol;
    lol += "// This file is reflected from code.\n";
    lol += "#ifdef FAZE_DX12\n";
    lol += "#define VK_BINDING(index) \n";
    lol += "#else // vulkan\n";
    lol += "#define VK_BINDING(index) [[vk::binding(index)]]\n";
    lol += "#endif\n";
    lol += "\n";

    lol += "#define ROOTSIG \"RootFlags(0), CBV(b0), DescriptorTable("; // , DescriptorTable()";

    {
      int uavC = 0, srvC = 0;
      bool currentMode = true;
      int count = 0;
      bool madeATable = false;
      auto addTable = [&]()
      {
        if (count > 0)
        {
          if (madeATable)
          {
            lol += ",";
          }
          if (currentMode)
          {
            lol += "SRV(t" + std::to_string(srvC);
          }
          else
          {
            lol += "UAV(u" + std::to_string(uavC);
          }
          lol += ", numDescriptors = " + std::to_string(count) + ")";
          if (currentMode)
          {
            srvC += count;
          }
          else
          {
            uavC += count;
          }
          count = 0;
          madeATable = true;
        }
      };
      for (auto&& it : desc.sortedResources)
      {
        if (it.readonly != currentMode)
        {
          addTable();
          currentMode = it.readonly;
        }
        count++;
      }
      addTable();
    }
    lol += "), StaticSampler(s0, "                   \
    "filter = FILTER_MIN_MAG_LINEAR_MIP_POINT), " \
    "StaticSampler(s1, " 									        \
    "filter = FILTER_MIN_MAG_MIP_POINT), " 				\
    "StaticSampler(s2, " 									        \
    "filter = FILTER_MIN_MAG_LINEAR_MIP_POINT, " 	\
    "addressU = TEXTURE_ADDRESS_WRAP, " 					\
    "addressV = TEXTURE_ADDRESS_WRAP, " 					\
    "addressW = TEXTURE_ADDRESS_WRAP), " 					\
    "StaticSampler(s3, " 									        \
    "filter = FILTER_MIN_MAG_MIP_POINT, " 				\
    "addressU = TEXTURE_ADDRESS_WRAP, " 					\
    "addressV = TEXTURE_ADDRESS_WRAP, " 					\
    "addressW = TEXTURE_ADDRESS_WRAP)\"\n\n"; 			\

    int gi = 0;
    if (!desc.constantStructBody.empty())
    {
      gi++;
      lol += "struct Constants\n{" + desc.constantStructBody + " };\n";
      lol += "VK_BINDING(0) ConstantBuffer<Constants> constants : register( b0 );\n";
    }
    
    auto resourceToString = [](bool writeRes, int gindex, int index, ShaderResource res) -> std::string
    {
      std::string resOut;
      resOut = "VK_BINDING("+ std::to_string(gindex) + ") ";
      if (writeRes)
      {
        resOut += "RW";
      }
      resOut += toString(res.type);
      if (!res.templateParameter.empty())
      {
        resOut += "<" + res.templateParameter + ">";
      }
      resOut += " " + res.name + " : register( ";
      if (writeRes)
      {
        resOut += "u";
      }
      else
      {
        resOut += "t";
      }
      resOut += std::to_string(index) + " );";
      return resOut;
    };
    
    int i = 0;
    
    if (!desc.structDecls.empty()) lol += "// Struct declarations\n";
    for (auto&& strct : desc.structDecls)
    {
      lol += strct + "\n";
    }
    
    lol += "\n// Read Only resources\n";
    for (auto&& res : desc.sortedResources)
    {
      if (res.readonly)
        lol += resourceToString(false, gi, i++, res) + "\n";
      gi++;
    }
    i = 0;
    gi = 1;
    lol += "\n// ReadWrite resources\n";
    for (auto&& res : desc.sortedResources)
    {
      if (!res.readonly)
        lol += resourceToString(true, gi, i++, res) + "\n";
      gi++;
    }
    
    return lol;
  }
}
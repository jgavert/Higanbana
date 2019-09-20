#include "higanbana/graphics/desc/shader_interface_descriptor.hpp"

namespace higanbana
{
  std::string hlslTablesFromShaderArguments(int setNumber, ShaderArgumentsLayout& args)
  {
    std::string lol;
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
            lol += ",\\\n    ";
          }
          else
          {
            lol += "DescriptorTable(\\\n    ";
          }
          
          if (currentMode)
          {
            lol += " SRV(t" + std::to_string(srvC);
          }
          else
          {
            lol += " UAV(u" + std::to_string(uavC);
          }
          lol += ", numDescriptors = " + std::to_string(count) + ", space=" + std::to_string(setNumber) + " )";
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
      for (auto&& it : args.resources())
      {
        if (it.readonly != currentMode)
        {
          addTable();
          currentMode = it.readonly;
        }
        count++;
      }
      addTable();
      if (madeATable)
      {
        lol += "),\\\n  ";
      }
    }
    return lol;
  }

  void resourceDeclarations(std::string& lol, int set, ShaderArgumentsLayout& args)
  {
    auto sset = ", " + std::to_string(set);
    auto sspace = ", space" + std::to_string(set);
    auto resourceToString = [sset, sspace](bool writeRes, int gindex, int index, ShaderResource res) -> std::string
    {
      std::string resOut;
      resOut = "VK_BINDING("+ std::to_string(gindex) + sset + ") ";
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
      resOut += std::to_string(index) + sspace + " );";
      return resOut;
    };
    
    int i = 0;
    int gi = 0;

    lol += "// Shader Arguments " + std::to_string(set) + "\n";
    
    if (!args.structs().empty()) lol += "// Struct declarations\n";
    for (auto&& strct : args.structs())
    {
      lol += strct + "\n";
    }
    
    lol += "\n// Read Only resources\n";
    for (auto&& res : args.resources())
    {
      if (res.readonly)
      {
        lol += resourceToString(false, gi, i++, res) + "\n";
        gi++;
      }
    }
    i = 0;
    //gi = 1;
    lol += "\n// Read Write resources\n";
    for (auto&& res : args.resources())
    {
      if (!res.readonly)
      {
        lol += resourceToString(true, gi, i++, res) + "\n";
        gi++;
      }
    }
  }

  std::string ShaderInterfaceDescriptor::createInterface()
  {
    std::string lol;
    lol += "// This file is generated from code.\n";
    lol += "#ifdef HIGANBANA_VULKAN\n";
    lol += "#define VK_BINDING(index, set) [[vk::binding(index, set)]]\n";
    lol += "#else // HIGANBANA_DX12\n";
    lol += "#define VK_BINDING(index, set) \n";
    lol += "#endif\n";
    lol += "\n";

    lol += "#define ROOTSIG \"RootFlags(0), \\\n  CBV(b0), \\\n  "; // , DescriptorTable()";

    int set = 0;
    for (auto&& arg : m_sets)
    {
      if (arg.handle().id != ResourceHandle::InvalidId)
      {
        lol += hlslTablesFromShaderArguments(set, arg);
      }
      set++;
    }

    // ??
    lol += "StaticSampler(s0, "                   \
    "filter = FILTER_MIN_MAG_LINEAR_MIP_POINT), " \
    "\\\n  StaticSampler(s1, " 									        \
    "filter = FILTER_MIN_MAG_MIP_POINT), " 				\
    "\\\n  StaticSampler(s2, " 									        \
    "filter = FILTER_MIN_MAG_LINEAR_MIP_POINT, " 	\
    "addressU = TEXTURE_ADDRESS_WRAP, " 					\
    "addressV = TEXTURE_ADDRESS_WRAP, " 					\
    "addressW = TEXTURE_ADDRESS_WRAP), " 					\
    "\\\n  StaticSampler(s3, " 									        \
    "filter = FILTER_MIN_MAG_MIP_POINT, " 				\
    "addressU = TEXTURE_ADDRESS_WRAP, " 					\
    "addressV = TEXTURE_ADDRESS_WRAP, " 					\
    "addressW = TEXTURE_ADDRESS_WRAP)\"\n\n"; 			\

    int gi = 0;
    if (!constantStructBody.empty())
    {
      gi++;
      lol += "struct Constants\n{" + constantStructBody + " };\n";
      lol += "VK_BINDING(0, " + std::to_string(m_sets.size()) + ") ConstantBuffer<Constants> constants : register( b0 );\n";
    }
    set = 0;
    for (auto&& arg : m_sets)
    {
      if (arg.handle().id != ResourceHandle::InvalidId)
      {
        resourceDeclarations(lol, set, arg);
        //gi += arg.resources().size();
      }
      set++;
    }
    

    lol += "\n// Usable Static Samplers\n";
    lol += "VK_BINDING(" + std::to_string(gi++) + ", " + std::to_string(set) + ") SamplerState bilinearSampler : register( s0 );\n";
    lol += "VK_BINDING(" + std::to_string(gi++) + ", " + std::to_string(set) + ") SamplerState pointSampler : register( s1 );\n";
    lol += "VK_BINDING(" + std::to_string(gi++) + ", " + std::to_string(set) + ") SamplerState bilinearSamplerWarp : register( s2 );\n";
    lol += "VK_BINDING(" + std::to_string(gi++) + ", " + std::to_string(set) + ") SamplerState pointSamplerWrap : register( s3 );\n";
    
    return lol;
  }
}
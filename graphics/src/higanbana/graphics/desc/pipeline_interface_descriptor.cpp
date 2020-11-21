#include "higanbana/graphics/desc/pipeline_interface_descriptor.hpp"

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
      auto addTable = [&](bool bindless)
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
          lol += ", numDescriptors = ";
          if (!bindless)
            lol += std::to_string(count) + ", space=" + std::to_string(setNumber) + " )";
          else
            lol += "unbounded, space=" + std::to_string(setNumber) +", flags=DESCRIPTORS_VOLATILE )";
          
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
          addTable(false);
          currentMode = it.readonly;
        }
        count++;
      }
      addTable(false);
      auto bindless = args.bindless();
      if (!bindless.name.empty())
      {
        currentMode = bindless.readonly;
        count++;
        addTable(true);
      }
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
      resOut += " " + res.name;
      if (res.bindless)
        resOut += "[]";
      resOut += " : register( ";
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
    
    int srv = 0;
    int uav = 0;
    int gi = 0;

    lol += "// Shader Arguments " + std::to_string(set) + "\n";
    
    if (!args.structs().empty()) lol += "// Struct declarations\n";
    unordered_set<size_t> filterAddedStructs;
    for (auto&& strct : args.structs())
    {
      if (filterAddedStructs.find(strct.first) != filterAddedStructs.end())
        continue;
      lol += strct.second + "\n";
      filterAddedStructs.insert(strct.first);
    }
    
    lol += "\n// Read Only resources\n";
    for (auto&& res : args.resources())
    {
      if (res.readonly)
      {
        lol += resourceToString(false, gi, srv++, res) + "\n";
        gi++;
      }
    }
    //gi = 1;
    lol += "\n// Read Write resources\n";
    for (auto&& res : args.resources())
    {
      if (!res.readonly)
      {
        lol += resourceToString(true, gi, uav++, res) + "\n";
        gi++;
      }
    }

    auto bindless = args.bindless();
    if (!bindless.name.empty())
    {
      lol += "\n// Bindless\n";
      lol += resourceToString(!bindless.readonly, gi, bindless.readonly ? srv : uav, bindless) + "\n";
    }
  }

  std::string PipelineInterfaceDescriptor::createInterface()
  {
    std::string lol;
    lol += "// This file is generated from code.\n";
    lol += "#ifdef HIGANBANA_VULKAN\n";
    lol += "#define VK_BINDING(index, set) [[vk::binding(index, set)]]\n";
    lol += "#else // HIGANBANA_DX12\n";
    lol += "#define VK_BINDING(index, set) \n";
    lol += "#endif\n";
    lol += "\n";

    lol += "#define ROOTSIG \"RootFlags(0), \\\n  CBV(b0), \\\n"; // , DescriptorTable()";
    lol += "  DescriptorTable( UAV(u99, numDescriptors = 1, space=99 )), \\\n  "; 

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
    "filter = FILTER_MIN_MAG_LINEAR_MIP_POINT, " \
    "addressU = TEXTURE_ADDRESS_CLAMP, "           \
    "addressV = TEXTURE_ADDRESS_CLAMP, "           \
    "addressW = TEXTURE_ADDRESS_CLAMP), "          \
    "\\\n  StaticSampler(s1, "                    \
    "filter = FILTER_MIN_MAG_MIP_POINT, "        \
    "addressU = TEXTURE_ADDRESS_CLAMP, "           \
    "addressV = TEXTURE_ADDRESS_CLAMP, "           \
    "addressW = TEXTURE_ADDRESS_CLAMP), "          \
    "\\\n  StaticSampler(s2, "                    \
    "filter = FILTER_MIN_MAG_LINEAR_MIP_POINT, "  \
    "addressU = TEXTURE_ADDRESS_WRAP, "           \
    "addressV = TEXTURE_ADDRESS_WRAP, "           \
    "addressW = TEXTURE_ADDRESS_WRAP), "          \
    "\\\n  StaticSampler(s3, "                    \
    "filter = FILTER_MIN_MAG_MIP_POINT, "         \
    "addressU = TEXTURE_ADDRESS_WRAP, "           \
    "addressV = TEXTURE_ADDRESS_WRAP, "           \
    "addressW = TEXTURE_ADDRESS_WRAP)\"\n\n";     \

    int gi = 0;
    for (auto&& structDecl : m_structs) {
      lol += structDecl + ";\n";
    }
    if (!constantStructBody.empty())
    {
      gi++;
      lol += "struct Constants\n{" + constantStructBody + " };\n";
      lol += "VK_BINDING(0, " + std::to_string(m_sets.size()) + ") ConstantBuffer<Constants> constants : register( b0 );\n";
      gi++;
      lol += "VK_BINDING(1, " + std::to_string(m_sets.size()) + ") RWByteAddressBuffer _debugOut : register( u99, space99 );\n";
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
    lol += "VK_BINDING(" + std::to_string(gi++) + ", " + std::to_string(set) + ") SamplerState pointSamplerWrap : register( s3 );\n\n";
    lol += R"(uint getIndex(uint count, uint type)
{
	uint myIndex;
	_debugOut.InterlockedAdd(0, count+1, myIndex);
	_debugOut.Store(myIndex*4+4, type);
	return myIndex*4+4+4;
}
void print(uint val)   { _debugOut.Store( getIndex(1, 1), val); }
void print(uint2 val)  { _debugOut.Store2(getIndex(2, 2), val); }
void print(uint3 val)  { _debugOut.Store3(getIndex(3, 3), val); }
void print(uint4 val)  { _debugOut.Store4(getIndex(4, 4), val); }
void print(int val)    { _debugOut.Store( getIndex(1, 5), asuint(val)); }
void print(int2 val)   { _debugOut.Store2(getIndex(2, 6), asuint(val)); }
void print(int3 val)   { _debugOut.Store3(getIndex(3, 7), asuint(val)); }
void print(int4 val)   { _debugOut.Store4(getIndex(4, 8), asuint(val)); }
void print(float val)  { _debugOut.Store( getIndex(1, 9), asuint(val)); }
void print(float2 val) { _debugOut.Store2(getIndex(2, 10), asuint(val)); }
void print(float3 val) { _debugOut.Store3(getIndex(3, 11), asuint(val)); }
void print(float4 val) { _debugOut.Store4(getIndex(4, 12), asuint(val)); }
)";
    
    return lol;
  }
}
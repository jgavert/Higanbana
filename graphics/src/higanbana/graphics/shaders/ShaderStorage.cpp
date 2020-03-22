#include "higanbana/graphics/shaders/ShaderStorage.hpp"
#include <higanbana/core/external/SpookyV2.hpp>
#include <string_view>

namespace higanbana
{
  namespace backend
  {    
    ShaderStorage::ShaderStorage(FileSystem& fs, std::shared_ptr<ShaderCompiler> compiler, std::string shaderPath, std::string binaryPath, ShaderBinaryType type)
      : m_fs(fs)
      , sourcePath("/" + shaderPath + "/")
      , compiledPath("/" + binaryPath + "/")
      , m_type(type)
      , m_compiler(compiler)
    {
      m_fs.loadDirectoryContentsRecursive(sourcePath);
    }

    std::string ShaderStorage::sourcePathCombiner(std::string shaderName, ShaderType type)
    {
      return sourcePath + shaderName + "." + shaderFileType(type) + ".hlsl";
    }

    std::string ShaderStorage::binaryPathCombiner(std::string shaderName, ShaderType type, uint3 tgs, std::vector<std::string> definitions)
    {
      std::string shaderBinType = toString(m_type);
      shaderBinType += "/";
      std::string shaderExtension = ".";
      shaderExtension += toString(m_type);

      std::string binType = "Release/";
#if defined(DEBUG)
      binType = "Debug/";
#endif

      std::string additionalBinInfo = "";

      if (type == ShaderType::Compute)
      {
        additionalBinInfo += ".";
        additionalBinInfo += std::to_string(tgs.x) + "_";
        additionalBinInfo += std::to_string(tgs.y) + "_";
        additionalBinInfo += std::to_string(tgs.z);
      }

      // TODO: instead of embedding all definitions to filename, create hash
      for (auto&& it : definitions)
      {
        additionalBinInfo += ".";
        additionalBinInfo += it;
      }

      return compiledPath + shaderBinType + binType + shaderName + additionalBinInfo + "." + shaderFileType(type) + shaderExtension;
    }

    std::string shaderStubFile(ShaderCreateInfo info)
    {
      std::string superSimple = "#include \"" + info.desc.shaderName + ".if.hlsl\"\n";
      superSimple += "// this is trying to be ";
      std::string inputOutput = "";
      switch (info.desc.type)
      {
      case ShaderType::Vertex:
        superSimple += "Vertex";
        inputOutput = "struct VertexOut\n\
{\n\
  float2 uv : TEXCOORD0;\n\
  float4 pos : SV_Position;\n\
};\n";
        break;
      case ShaderType::Pixel:
        superSimple += "Pixel";
        inputOutput = "struct VertexOut \
{ \
  float2 uv : TEXCOORD0; \
  float4 pos : SV_Position; \
};";
        break;
      case ShaderType::Compute:
        superSimple += "Compute";
        break;
      case ShaderType::Geometry:
        superSimple += "Geometry";
        break;
      case ShaderType::Hull: // hull?
        superSimple += "Hull(hull?)";
        break;
      case ShaderType::Domain: // domain?
        superSimple += "Domain(domain?)";
        break;
      default:
        HIGAN_ASSERT(false, "Unknown ShaderType");
      }
      superSimple += " shader file.\n";

      if (info.desc.type != ShaderType::Compute)
      {
        superSimple += inputOutput;
      }

      superSimple += "\n\n[RootSignature(ROOTSIG)]\n";
      if (info.desc.type == ShaderType::Compute)
      {
        superSimple += "[numthreads(HIGANBANA_THREADGROUP_X, HIGANBANA_THREADGROUP_Y, HIGANBANA_THREADGROUP_Z)] // @nolint\n";
        superSimple += "void main(uint2 id : SV_DispatchThreadID, uint2 gid : SV_GroupThreadID)\n{ \n\n\n}\n";
      }
      else
      {
        if (info.desc.type == ShaderType::Vertex)
        {
          superSimple += "VertexOut main(uint id: SV_VertexID)\n{ \n  VertexOut vtxOut;\n";
          superSimple += "  vtxOut.pos.x = (id % 3 == 2) ?  1 : 0;\n";
          superSimple += "  vtxOut.pos.y = (id % 3 == 1) ?  1 : 0;\n";
          superSimple += "  vtxOut.pos.z = 0;\n";
          superSimple += "  vtxOut.pos.w = 1.f;\n";
          superSimple += "  vtxOut.uv.x = (id % 3 == 2) ?  1 : 0;\n";
          superSimple += "  vtxOut.uv.y = (id % 3 == 1) ?  1 : 0;\n";
          superSimple += "  return vtxOut;\n";
          superSimple += "}\n";
        }
        else if (info.desc.type == ShaderType::Pixel)
        {
          superSimple += "float4 main(VertexOut input) : SV_TARGET\n{\n  return float4(input.uv, 0, 1);\n}\n";
        }
        else
        {
          HIGAN_ASSERT(false, "don't know what to generate :D");
        }
      }
      return superSimple;
    }

    bool isInterfaceValid(higanbana::FileSystem& fs, const std::string& interfacePath, const std::string& interfaceData)
    {
      SpookyHash hash;
      hash.Init(1337, 715517);
      hash.Update(interfaceData.data(), interfaceData.size());
      uint64_t h1, h2;
      hash.Final(&h1, &h2);
      bool needsReplacing = true;
      if (fs.fileExists(interfacePath))
      {
        auto fileView = fs.readFile(interfacePath);
        std::string_view view(reinterpret_cast<char*>(fileView.data()), fileView.size());
        uint64_t t1, t2;
        auto v = view.find_first_of("INTERFACE_HASH");
        if (v != std::string_view::npos)
        {
          auto startPos = view.find_first_of(":", v);
          auto middlePos = view.find_first_of(":", startPos+1);
          auto endPos = view.find_first_of("\n", middlePos);
          auto firstHash = std::string(view.data()+startPos+1, middlePos - startPos-1);
          auto secondHash = std::string(view.data()+middlePos+1, endPos - middlePos-1);
          t1 = std::stoull(firstHash);
          t2 = std::stoull(secondHash);
          needsReplacing = h1 != t1 || h2 != t2;
        }
      }
      if (needsReplacing) // if interface old, replace:
      {
        std::string addedHash = "// INTERFACE_HASH:" + std::to_string(h1) + ":" + std::to_string(h2)+"\n";
        addedHash += interfaceData;
        fs.writeFile(interfacePath, makeByteView(addedHash.data(), addedHash.size()));
        HIGAN_LOG("ShaderStorage", "Shader interfacefile \"%s\" was old/missing. Created new one.", interfacePath.c_str());
        // HIGAN_LOG("ShaderStorage", "Made a shader file(%s) for if: %s shader", shaderInterfacePath.c_str(), );
      }
      return !needsReplacing;
    }

    void ShaderStorage::ensureShaderSourceFilesExist(ShaderCreateInfo info)
    {
      // check situation first
      auto shaderInterfacePath = sourcePath + info.desc.shaderName + ".if.hlsl";
      auto shaderPath = sourcePathCombiner(info.desc.shaderName, info.desc.type);
      // just overwrite interface for now...
      // just output the interface from shaderCreateInfo generated by ShaderInputDescriptor
      isInterfaceValid(m_fs, shaderInterfacePath, info.desc.interfaceDeclaration);

      if (!m_fs.fileExists(shaderPath))
      {
        // hurr, stub shader ... I need create them manually first and then generate them.
        // maybe assert on what I haven't done yet.
        std::string superSimple = shaderStubFile(info); 
        m_fs.writeFile(shaderPath, makeByteView(superSimple.data(), superSimple.size()));

        HIGAN_LOG("ShaderStorage", "Made a shader file(%s) for %s: %s shader", shaderInterfacePath.c_str(), shaderFileType(info.desc.type), info.desc.shaderName.c_str());
      }
    }

    higanbana::MemoryBlob ShaderStorage::shader(ShaderCreateInfo info)
    {
      auto shaderPath = sourcePathCombiner(info.desc.shaderName, info.desc.type);
      auto dxilPath = binaryPathCombiner(info.desc.shaderName, info.desc.type, info.desc.tgs, info.desc.definitions);

      auto func = [&](std::string filename)
      {
        //HIGAN_LOG("included: %s\n", filename.c_str());
        m_fs.addWatchDependency(filename, shaderPath);
      };
      ensureShaderSourceFilesExist(info);
      auto foundBinary = m_fs.fileExists(dxilPath);
      if (!foundBinary)
      {
        //      HIGAN_ILOG("ShaderStorage", "First time compiling \"%s\"", shaderName.c_str());
        //HIGAN_ASSERT(compileShader(shaderName, type, tgs), "ups");
        HIGAN_ASSERT(m_compiler, "no compiler");
        auto compiled = m_compiler->compileShader(
          m_type,
          shaderPath,
          dxilPath,
          info,
          func);
        HIGAN_ASSERT(compiled, "ups");
      }
      foundBinary = m_fs.fileExists(dxilPath);
      if (foundBinary && m_fs.fileExists(shaderPath))
      {
        auto shaderInterfacePath = sourcePath + info.desc.shaderName + ".if.hlsl";

        auto shaderTime = m_fs.timeModified(shaderPath);
        auto dxilTime = m_fs.timeModified(dxilPath);
        auto shaderInterfaceTime = dxilTime;

        if (m_fs.fileExists(shaderInterfacePath))
        {
          shaderInterfaceTime = m_fs.timeModified(shaderInterfacePath);
        }

        if (m_compiler && (info.desc.forceCompile || shaderTime > dxilTime || shaderInterfaceTime > dxilTime))
        {
          // HIGAN_ILOG("ShaderStorage", "Spirv was old, compiling: \"%s\"", shaderName.c_str());
          bool result = m_compiler->compileShader(
            m_type,
            shaderPath,
            dxilPath,
            info,
            func);
          if (!result)
          {
            HIGAN_ILOG("ShaderStorage", "Shader compile failed.\n");
          }
        }
      }
      HIGAN_ASSERT(m_fs.fileExists(dxilPath), "wtf???");
      auto shader = m_fs.readFile(dxilPath);
      return shader;
    }

    WatchFile ShaderStorage::watch(std::string shaderName, ShaderType type)
    {
      auto shd = sourcePathCombiner(shaderName, type);
      if (m_fs.fileExists(shd))
      {
        return m_fs.watchFile(shd);
      }
      return WatchFile{};
    }
  }
}
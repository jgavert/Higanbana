#pragma once
#include "ShaderStorage.hpp"

namespace faze
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

    faze::MemoryBlob ShaderStorage::shader(const std::string& shaderName, ShaderType type, std::string rootSignature, std::vector<std::string> definitions, uint3 tgs)
    {
      auto shaderPath = sourcePathCombiner(shaderName, type);
      auto dxilPath = binaryPathCombiner(shaderName, type, tgs, definitions);

      auto func = [](std::string filename)
      {
        F_LOG("included: %s\n", filename.c_str());
      };

      if (!m_fs.fileExists(dxilPath))
      {
        //      F_ILOG("ShaderStorage", "First time compiling \"%s\"", shaderName.c_str());
        //F_ASSERT(compileShader(shaderName, type, tgs), "ups");
        F_ASSERT(m_compiler, "no compiler");
        F_ASSERT(m_compiler->compileShader(m_type, shaderName, shaderPath, dxilPath, type, tgs, definitions, rootSignature, func), "ups");
      }
      if (m_fs.fileExists(dxilPath) && m_fs.fileExists(shaderPath))
      {
        auto shaderInterfacePath = sourcePath + shaderName + ".if.hpp";

        auto shaderTime = m_fs.timeModified(shaderPath);
        auto dxilTime = m_fs.timeModified(dxilPath);
        auto shaderInterfaceTime = dxilTime;

        if (m_fs.fileExists(shaderInterfacePath))
        {
          shaderInterfaceTime = m_fs.timeModified(shaderInterfacePath);
        }

        if (m_compiler && (shaderTime > dxilTime || shaderInterfaceTime > dxilTime))
        {
          // F_ILOG("ShaderStorage", "Spirv was old, compiling: \"%s\"", shaderName.c_str());
          bool result = m_compiler->compileShader(m_type, shaderName, shaderPath, dxilPath, type, tgs, definitions, rootSignature, func);
          if (!result)
          {
            F_ILOG("DX12", "Shader compile failed.\n");
          }
        }
      }
      F_ASSERT(m_fs.fileExists(dxilPath), "wtf???");
      auto shader = m_fs.readFile(dxilPath);
      return shader;
    }

    WatchFile ShaderStorage::watch(std::string shaderName, ShaderType type)
    {
      auto shd = sourcePath + shaderName + "." + shaderFileType(type) + ".hlsl";
      if (m_fs.fileExists(shd))
      {
        return m_fs.watchFile(shd);
      }
      return WatchFile{};
    }
  }
}
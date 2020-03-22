#pragma once
#include "higanbana/graphics/shaders/DXCompiler.hpp"
#include <higanbana/core/filesystem/filesystem.hpp>
#include <higanbana/core/global_debug.hpp>
#include <higanbana/core/math/math.hpp>
#include <higanbana/core/datastructures/proxy.hpp>

#include <fstream>
#include <string>
#include <memory>

namespace higanbana
{
  namespace backend
  {
    class ShaderStorage
    {
    public:
      const char* toString(ShaderBinaryType type)
      {
        return type == ShaderBinaryType::SPIRV ? "spirv" : "dxil";
      }
    private:
      FileSystem & m_fs;
      std::string sourcePath;
      std::string compiledPath;
      ShaderBinaryType m_type;
      std::shared_ptr<ShaderCompiler> m_compiler;
    public:
      ShaderStorage(FileSystem& fs, std::shared_ptr<ShaderCompiler> compiler, std::string shaderPath, std::string binaryPath, ShaderBinaryType type);
      std::string sourcePathCombiner(std::string shaderName, ShaderType type);
      std::string binaryPathCombiner(std::string shaderName, ShaderType type, uint3 tgs, std::vector<std::string> definitions);
	  void ensureShaderSourceFilesExist(ShaderCreateInfo info);
      higanbana::MemoryBlob shader(ShaderCreateInfo info);
      WatchFile watch(std::string shaderName, ShaderType type);
    };
  }
}
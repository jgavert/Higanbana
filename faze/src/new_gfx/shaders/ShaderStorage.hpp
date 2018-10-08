#pragma once
#include "core/src/filesystem/filesystem.hpp"
#include "core/src/global_debug.hpp"
#include "core/src/math/math.hpp"

#include "faze/src/new_gfx/shaders/DXCompiler.hpp"

#include <fstream>
#include <string>
#include <memory>

namespace faze
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
      faze::MemoryBlob shader(const std::string& shaderName, ShaderType type, std::string rootSignature, std::vector<std::string> definitions = {}, uint3 tgs = uint3(1, 1, 1));
      WatchFile watch(std::string shaderName, ShaderType type);
    };
  }
}
#pragma once

#include "core/src/filesystem/filesystem.hpp"
#include "core/src/global_debug.hpp"

namespace faze
{
  namespace backend
  {
    class DX12ShaderStorage
    {
    private:
      FileSystem& m_fs;
      std::string sourcePath;
      std::string compiledPath;
    public:
      enum class ShaderType
      {
        Vertex,
        Pixel,
        Compute,
        Geometry,
        TessControl,
        TessEvaluation
      };
    private:

      const char* shaderFileType(ShaderType type)
      {
        switch (type)
        {
        case ShaderType::Vertex:
          return "vs.glsl";
        case ShaderType::Pixel:
          return "ps.glsl";
        case ShaderType::Compute:
          return "cs.glsl";
        case ShaderType::Geometry:
          return "gs.glsl";
        case ShaderType::TessControl:
          return "tc.glsl";
        case ShaderType::TessEvaluation:
          return "te.glsl";
        default:
          F_ASSERT(false, "Unknown ShaderType");
        }
        return "";
      }

    public:
      DX12ShaderStorage(FileSystem& fs, std::string shaderPath, std::string binaryPath)
        : m_fs(fs)
        , sourcePath("/" + shaderPath + "/")
        , compiledPath("/" + binaryPath + "/")
      {
        m_fs.loadDirectoryContentsRecursive(sourcePath);
        // we could compile all shaders that don't have spv ahead of time
        // requires support from filesystem
      }

      bool compileShader(std::string, ShaderType)
      {
        return false;
      }

      bool shader(vk::Device&, std::string, ShaderType)
      {
        return false;
      }
    };
  }
}
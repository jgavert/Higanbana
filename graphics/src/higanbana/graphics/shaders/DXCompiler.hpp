#pragma once

#include <higanbana/core/platform/definitions.hpp>
#include "higanbana/graphics/desc/shader_desc.hpp"
#include <higanbana/core/filesystem/filesystem.hpp>
#include <higanbana/core/math/math.hpp>
#include <higanbana/core/global_debug.hpp>
#include <fstream>
#include <string>


namespace higanbana
{
  namespace backend
  {
    const char* shaderFileType(ShaderType type);

    class ShaderCompiler
    {
    public:
      virtual bool compileShader(
        ShaderBinaryType binType,
        std::string shaderSourcePath,
        std::string shaderBinaryPath,
        ShaderCreateInfo info,
        std::function<void(std::string)> includeCallback) = 0;
    };
#if defined(HIGANBANA_PLATFORM_WINDOWS)
    class DXCompiler : public ShaderCompiler
    {
      higanbana::FileSystem& m_fs;
      std::string m_sourcePath;

      const wchar_t* shaderFeatureDXC(ShaderType type)
      {
        switch (type)
        {
        case ShaderType::Vertex:
          return L"vs_6_4";
        case ShaderType::Pixel:
          return L"ps_6_4";
        case ShaderType::Compute:
          return L"cs_6_4";
        case ShaderType::Geometry:
          return L"gs_6_4";
        case ShaderType::Hull: // hs_6_0 ??
          return L"hs_6_4";
        case ShaderType::Domain: // ds_6_0 ??
          return L"ds_6_4";
        case ShaderType::Amplification: //
          return L"as_6_5";
        case ShaderType::Mesh: //
          return L"ms_6_5";
        default:
          HIGAN_ASSERT(false, "Unknown ShaderType");
        }
        return L"";
      }
    public:
      DXCompiler(FileSystem& files, std::string sourcePath)
        : m_fs(files)
        , m_sourcePath(sourcePath)
      {}

      virtual bool compileShader(
        ShaderBinaryType binType,
        std::string shaderSourcePath,
        std::string shaderBinaryPath,
        ShaderCreateInfo info,
        std::function<void(std::string)> includeCallback);
    };
#endif
  }
}
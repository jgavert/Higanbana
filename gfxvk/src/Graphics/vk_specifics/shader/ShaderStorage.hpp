#pragma once
#include "core/src/filesystem/filesystem.hpp"
#include <vulkan/vk_cpp.h>
#include <shaderc/shaderc.hpp> 


class ShaderStorage
{
private:
  FileSystem m_fs;
  std::string basePath;


public:
  enum class ShaderType
  {
    Vertex = shaderc_glsl_vertex_shader,
    Pixel = shaderc_glsl_fragment_shader,
    Compute = shaderc_glsl_compute_shader,
    Geometry = shaderc_glsl_geometry_shader,
    TessControl = shaderc_glsl_tess_control_shader,
    TessEvaluation = shaderc_glsl_tess_evaluation_shader
  };
private:

  const char* shaderFileType(ShaderType type)
  {
    switch (type)
    {
    case ShaderType::Vertex:
      return "vs";
    case ShaderType::Pixel:
      return "ps";
    case ShaderType::Compute:
      return "cs";
    case ShaderType::Geometry:
      return "gs";
    case ShaderType::TessControl:
      return "tc";
    case ShaderType::TessEvaluation:
      return "te";
    default:
      F_ASSERT(false, "Unknown ShaderType");
    }
    return "";
  }

public:

  ShaderStorage(std::string shaderPath)
    : m_fs(shaderPath)
    , basePath(shaderPath.substr(1))
  {
  }

  bool compileShader(std::string shaderName, ShaderType type)
  {
    auto fullPath = basePath + "/" + shaderName + "." + shaderFileType(type);
    F_ASSERT(m_fs.fileExists(fullPath), "Shader file doesn't exists in path %c\n", fullPath.c_str());
    auto blob = m_fs.readFile(fullPath);
    std::string text;
    text.resize(blob.size());
    memcpy(reinterpret_cast<char*>(&text[0]), blob.data(), blob.size());
    shaderc::CompileOptions opt;
    shaderc::Compiler compiler;
    auto something = compiler.CompileGlslToSpv(text, shaderc_glsl_compute_shader, "main", opt);
    if (something.GetCompilationStatus() != shaderc_compilation_status_success)
    {
      auto asd = something.GetErrorMessage();
      F_ILOG("ShaderStorage", "Compilation failed. trying to print error...\n");
      std::string asdError = "";
      int lastEnd = 0;
      for (int i = 0; i < asd.size(); ++i)
      {
        if (asd[i] < 0)
        {
          //asd[i] = ' ';
        }
        if (asd[i] == '\n')
        {
          asdError = asd.substr(lastEnd, i - lastEnd);
          lastEnd = i + 1;
          ++i;
          F_ILOG("ShaderStorage", "%s\n", asdError.c_str());
        }
      }
      //F_SLOG("SHADERC", "%s\n", asd.c_str());
      return false;
    }
    else
    {
      size_t length_in_words = something.cend() - something.cbegin();
      F_ILOG("ShaderStorage", "Compiled unseen shader %s.\n", shaderName.c_str());
      m_fs.writeFile(fullPath + ".spv", something.cbegin(), length_in_words * 4);
      m_fs.flushFiles();
    }
    return true;
  }

  vk::ShaderModule shader(vk::Device& device, std::string shaderName, ShaderType type)
  {
    auto fullPath = basePath + "/" + shaderName + "." + shaderFileType(type) + ".spv";
    if (!m_fs.fileExists(fullPath) || true)
    {
      F_ASSERT(compileShader(shaderName, type), "ups");
    }
    F_ASSERT(m_fs.fileExists(fullPath), "wtf???");
    auto shader = m_fs.readFile(fullPath);
    vk::ShaderModuleCreateInfo moduleCreate = vk::ShaderModuleCreateInfo()
      .pCode(reinterpret_cast<uint32_t*>(shader.data()))
      .codeSize(shader.size());

    return device.createShaderModule(moduleCreate);
  }
};

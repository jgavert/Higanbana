#pragma once
#include "core/src/filesystem/filesystem.hpp"
#include "core/src/global_debug.hpp"
#include <vulkan/vulkan.hpp>
#include <shaderc/shaderc.hpp> 


class ShaderStorage
{
private:
  FileSystem& m_fs;
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

  ShaderStorage(FileSystem& fs, std::string shaderPath)
    : m_fs(fs)
    , basePath(shaderPath.substr(1))
  {
  }

  bool compileShader(std::string shaderName, ShaderType type)
  {
    auto shaderPath = basePath + "/" + shaderName + "." + shaderFileType(type);
    auto spvPath = basePath + "/spv/" + shaderName + "." + shaderFileType(type) + ".spv";

    F_ASSERT(m_fs.fileExists(shaderPath), "Shader file doesn't exists in path %c\n", shaderPath.c_str());
    auto blob = m_fs.readFile(shaderPath);
    std::string text;
    text.resize(blob.size());
    memcpy(reinterpret_cast<char*>(&text[0]), blob.data(), blob.size());
    //printf("%s\n", text.data());
    //text.erase(std::remove(text.begin(), text.end(), '\0'), text.end());
    shaderc::CompileOptions opt;
    shaderc::Compiler compiler;
    auto something = compiler.CompileGlslToSpv(text, static_cast<shaderc_shader_kind>(type), "main", opt);
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
          F_ILOG("ShaderStorage", "%s", asdError.c_str());
        }
      }
      //F_SLOG("SHADERC", "%s\n", asd.c_str());
      return false;
    }
    else
    {
      auto shader = m_fs.readFile(spvPath);
      size_t length_in_words = something.cend() - something.cbegin();
      F_ILOG("ShaderStorage", "Compiled: \"%s\"", shaderName.c_str());
      m_fs.writeFile(spvPath, something.cbegin(), length_in_words);
      auto shader2 = m_fs.readFile(spvPath);

      uint32_t* origPtr = reinterpret_cast<uint32_t*>(shader.data());
      uint32_t* nextPtr = reinterpret_cast<uint32_t*>(shader2.data());
      if (shader.size() != shader2.size())
      {
        F_ILOG("ShaderStorage", "difference in bytes %zu vs %zu ", shader.size(), shader2.size());
      }
      for (size_t i = 0; i < shader.size()/4; ++i)
      {
        if (origPtr[i] != nextPtr[i])
        {
          F_ILOG("ShaderStorage", "%zu: difference in bytes %u vs %u ", i, origPtr[i], nextPtr[i]);
        }
      }
    }
    return true;
  }

  vk::ShaderModule shader(vk::Device& device, std::string shaderName, ShaderType type)
  {
    auto shaderPath = basePath + "/" + shaderName + "." + shaderFileType(type);
    auto spvPath = basePath + "/spv/" + shaderName + "." + shaderFileType(type) + ".spv";
    
    if (!m_fs.fileExists(spvPath))
    {
//      F_ILOG("ShaderStorage", "First time compiling \"%s\"", shaderName.c_str());
      F_ASSERT(compileShader(shaderName, type), "ups");
    }
    if (m_fs.fileExists(spvPath))
    {
      auto shaderTime = m_fs.timeModified(shaderPath);
      auto spirvTime = m_fs.timeModified(spvPath);

      if (shaderTime > spirvTime)
      {
//        F_ILOG("ShaderStorage", "Spirv was old, compiling: \"%s\"", shaderName.c_str());
        F_ASSERT(compileShader(shaderName, type), "ups");
      }
    }
    F_ASSERT(m_fs.fileExists(spvPath), "wtf???");
    auto shader = m_fs.readFile(spvPath);
    vk::ShaderModuleCreateInfo moduleCreate = vk::ShaderModuleCreateInfo()
      .setPCode(reinterpret_cast<uint32_t*>(shader.data()))
      .setCodeSize(shader.size());

    return device.createShaderModule(moduleCreate);
  }
};
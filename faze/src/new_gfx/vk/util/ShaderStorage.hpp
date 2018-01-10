#pragma once

#include "core/src/filesystem/filesystem.hpp"
#include "core/src/global_debug.hpp"
#include "faze/src/new_gfx/vk/vulkan.hpp"
#include <shaderc/shaderc.hpp>

class ShaderStorage
{
private:
  faze::FileSystem& m_fs;
  std::string sourcePath;
  std::string compiledPath;
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
      return "vs.hlsl";
    case ShaderType::Pixel:
      return "ps.hlsl";
    case ShaderType::Compute:
      return "cs.hlsl";
    case ShaderType::Geometry:
      return "gs.hlsl";
    case ShaderType::TessControl:
      return "tc.hlsl";
    case ShaderType::TessEvaluation:
      return "te.hlsl";
    default:
      F_ASSERT(false, "Unknown ShaderType");
    }
    return "";
  }

public:

  ShaderStorage(faze::FileSystem& fs, std::string shaderPath, std::string spirvPath)
    : m_fs(fs)
    , sourcePath("/" + shaderPath + "/")
    , compiledPath("/" + spirvPath + "/")
  {
    m_fs.loadDirectoryContentsRecursive(sourcePath);
    // we could compile all shaders that don't have spv ahead of time
    // requires support from filesystem
  }

  class IncludeHelper : public shaderc::CompileOptions::IncluderInterface
  {
  private:
    faze::FileSystem& m_fs;
    std::string sourcePath;
  public:
    IncludeHelper(faze::FileSystem& fs, std::string sourcePath)
      : m_fs(fs)
      , sourcePath(sourcePath)
    {}

    shaderc_include_result* GetInclude(const char* requested_source, shaderc_include_type type, const char* requesting_source, size_t include_depth) override
    {
      F_ASSERT(include_depth < 5, "This doesn't sound like everything is alright. Otherwise increase.");
      F_ILOG("ShaderStorage", "Includer: Requested source \"%s\" include_type: %d requesting_source: \"%s\" include_depth: %zu", requested_source, type, requesting_source, include_depth);
      std::string filename(requested_source);
      std::string fullPath = sourcePath + filename;
      if (filename.size() > 15 && filename.compare(filename.size() - 15, 15, "definitions.hpp") == 0)
      {
        fullPath = "/../" + filename;
        if (!m_fs.fileExists(fullPath))
          m_fs.loadDirectoryContentsRecursive("/../app/graphics/");
      }
      F_ASSERT(m_fs.fileExists(fullPath), "File has to exist. for now.");

      auto sourceView = m_fs.viewToFile(fullPath);
      shaderc_include_result* result = (shaderc_include_result*)malloc(sizeof(shaderc_include_result));
      result->content = reinterpret_cast<const char*>(sourceView.data());
      result->content_length = sourceView.size();
      auto reqSrcLen = strlen(requested_source);
      char* lol = (char*)malloc(sizeof(char)*(reqSrcLen + 1));
      memcpy(lol, requested_source, reqSrcLen);
      lol[reqSrcLen] = '\0';
      result->source_name = lol;
      result->source_name_length = reqSrcLen + 1;
      result->user_data = lol;

      return result;
    }

    // Handles shaderc_include_result_release_fn callbacks.
    void ReleaseInclude(shaderc_include_result* usedResult) override
    {
      char* lol = reinterpret_cast<char*>(usedResult->user_data);
      free(lol);
      free(usedResult);
    }
  };

  std::string sourcePathCombiner(std::string shaderName, ShaderType type)
  {
    return sourcePath + shaderName + "." + shaderFileType(type);
  }

  std::string binaryPathCombiner(std::string shaderName, ShaderType type, uint3 tgs)
  {
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

    return compiledPath + binType + shaderName + additionalBinInfo + "." + shaderFileType(type) + ".spv";
  }

  bool compileShader(std::string shaderName, ShaderType type, uint3 tgs)
  {
    auto shaderPath = sourcePathCombiner(shaderName, type);
    auto spvPath = binaryPathCombiner(shaderName, type, tgs);

    F_ASSERT(m_fs.fileExists(shaderPath), "Shader file doesn't exists in path %c\n", shaderPath.c_str());
    auto view = m_fs.viewToFile(shaderPath);
    std::string text;
    text.resize(view.size());
    memcpy(reinterpret_cast<char*>(&text[0]), view.data(), view.size());
    /*
    printf("%s\n", text.data());
    OutputDebugStringA(text.data());
    */
    //text.erase(std::remove(text.begin(), text.end(), '\0'), text.end());

    shaderc::CompileOptions opt;
    opt.SetSourceLanguage(shaderc_source_language_hlsl);
    opt.SetIncluder(std::make_unique<IncludeHelper>(m_fs, sourcePath));
    //opt.SetForcedVersionProfile(450, shaderc_profile_none);
    //opt.SetTargetEnvironment(shaderc_target_env_vulkan, 100);
    opt.AddMacroDefinition("FAZE_VULKAN");

    auto TGS_X = std::to_string(tgs.x);
    auto TGS_Y = std::to_string(tgs.y);
    auto TGS_Z = std::to_string(tgs.z);
    opt.AddMacroDefinition("FAZE_THREADGROUP_X", TGS_X);
    opt.AddMacroDefinition("FAZE_THREADGROUP_Y", TGS_Y);
    opt.AddMacroDefinition("FAZE_THREADGROUP_Z", TGS_Z);
    shaderc::Compiler compiler;
    auto something = compiler.CompileGlslToSpv(text, static_cast<shaderc_shader_kind>(type), "main", opt);
    if (something.GetCompilationStatus() != shaderc_compilation_status_success)
    {
      auto asd = something.GetErrorMessage();
      F_ILOG("ShaderStorage", "Compilation failed. trying to print error...\n");
      std::string asdError = "";
      int lastEnd = 0;
      for (int i = 0; i < static_cast<int>(asd.size()); ++i)
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
      size_t length_in_words = something.cend() - something.cbegin();
      F_ILOG("ShaderStorage", "Compiled: \"%s\"", shaderName.c_str());
      m_fs.writeFile(spvPath, faze::reinterpret_memView<const uint8_t>(faze::makeMemView(something.cbegin(), length_in_words)));
    }
    return true;
  }

  vk::ShaderModule shader(vk::Device& device, std::string shaderName, ShaderType type, uint3 tgs = uint3(1, 1, 1))
  {
    auto shaderPath = sourcePathCombiner(shaderName, type);
    auto spvPath = binaryPathCombiner(shaderName, type, tgs);

    if (!m_fs.fileExists(spvPath))
    {
      //      F_ILOG("ShaderStorage", "First time compiling \"%s\"", shaderName.c_str());
      F_ASSERT(compileShader(shaderName, type, tgs), "ups");
    }
    if (m_fs.fileExists(spvPath))
    {
      auto shaderInterfacePath = sourcePath + shaderName + ".if.hpp";

      auto shaderTime = m_fs.timeModified(shaderPath);
      auto shaderInterfaceTime = m_fs.timeModified(shaderInterfacePath);
      auto spirvTime = m_fs.timeModified(spvPath);

      if (shaderTime > spirvTime || shaderInterfaceTime > spirvTime)
      {
        //        F_ILOG("ShaderStorage", "Spirv was old, compiling: \"%s\"", shaderName.c_str());
        F_ASSERT(compileShader(shaderName, type, tgs), "ups");
      }
    }
    F_ASSERT(m_fs.fileExists(spvPath), "wtf???");
    auto shader = m_fs.readFile(spvPath);
    vk::ShaderModuleCreateInfo moduleCreate = vk::ShaderModuleCreateInfo()
      .setPCode(reinterpret_cast<uint32_t*>(shader.data()))
      .setCodeSize(shader.size());

    auto shaderMod = device.createShaderModule(moduleCreate);

    return shaderMod;
  }
};
#pragma once

#include "core/src/filesystem/filesystem.hpp"
#include "core/src/global_debug.hpp"

#include "faze/src/new_gfx/dx12/dx12.hpp"

#include <fstream>
#include <string>

class CShaderInclude : public ID3DInclude {
public:
  CShaderInclude(faze::FileSystem& fs, std::string sourcePath) : m_fs(fs), m_sourcePath(sourcePath) {}

  HRESULT __stdcall Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID /*pParentData*/, LPCVOID *ppData, UINT *pBytes)
  {
    std::string filename(pFileName);
    std::string finalPath;
    if (filename.compare(filename.size() - 15, 15, "definitions.hpp") == 0)
    {
      finalPath = "/../" + filename;
      if (!m_fs.fileExists(finalPath))
        m_fs.loadDirectoryContentsRecursive("/../app/graphics/");
    }
    else
    {
      switch (IncludeType) {
      case D3D_INCLUDE_LOCAL:
        finalPath = m_sourcePath + filename;
        break;
      case D3D_INCLUDE_SYSTEM:
        finalPath = m_sourcePath + filename;
        break;
      default:
        assert(0);
      }
    }
    F_ASSERT(m_fs.fileExists(finalPath), "Shader file doesn't exists in path %s\n", finalPath.c_str());

    auto shader = m_fs.viewToFile(finalPath);
    *ppData = shader.data();
    *pBytes = static_cast<UINT>(shader.size());
    return S_OK;
  }
  HRESULT __stdcall Close(LPCVOID) {
    return S_OK;
  }
private:
  faze::FileSystem& m_fs;
  std::string m_sourcePath;
};

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
          return "vs.hlsl";
        case ShaderType::Pixel:
          return "ps.hlsl";
        case ShaderType::Compute:
          return "cs.hlsl";
        case ShaderType::Geometry:
          return "gs.hlsl";
        case ShaderType::TessControl: // hull?
          return "tc.hlsl";
        case ShaderType::TessEvaluation: // domain?
          return "te.hlsl";
        default:
          F_ASSERT(false, "Unknown ShaderType");
        }
        return "";
      }

      const char* shaderFeature(ShaderType type)
      {
        switch (type)
        {
        case ShaderType::Vertex:
          return "vs_5_1";
        case ShaderType::Pixel:
          return "ps_5_1";
        case ShaderType::Compute:
          return "cs_5_1";
        case ShaderType::Geometry:
          return "gs_5_1";
        case ShaderType::TessControl:
        case ShaderType::TessEvaluation:
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

      bool compileShader(std::string shaderName, ShaderType type)
      {
        auto shaderPath = sourcePath + shaderName + "." + shaderFileType(type);
        auto dxilPath = compiledPath + shaderName + "." + shaderFileType(type) + ".dxil";
        F_ASSERT(m_fs.fileExists(shaderPath), "Shader file doesn't exists in path %s\n", shaderPath.c_str());
        auto view = m_fs.viewToFile(shaderPath);
        std::string text;
        text.resize(view.size());
        memcpy(reinterpret_cast<char*>(&text[0]), view.data(), view.size());
        printf("%s\n", text.data());
        // we got shader in "text"

        CShaderInclude include(m_fs, sourcePath);
        auto p = shaderFeature(type);
        UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS | D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES;
#if !defined(RELEASE)
        compileFlags |= D3DCOMPILE_DEBUG;
#endif
#if !defined(DEBUG)
        compileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
        ComPtr<ID3DBlob> shaderBlob;
        ComPtr<ID3DBlob> errorMsg;

        HRESULT hr = D3DCompile(text.data(), text.size(), nullptr, nullptr, &include, "main", p, compileFlags, 0, shaderBlob.GetAddressOf(), errorMsg.GetAddressOf());
        // https://msdn.microsoft.com/en-us/library/dn859356(v=vs.85).aspx
        if (FAILED(hr))
        {
          if (errorMsg.Get())
          {
            OutputDebugStringA("Error In \"");
            OutputDebugStringA(shaderPath.c_str());
            OutputDebugStringA("\": \n");
            OutputDebugStringA((char*)errorMsg->GetBufferPointer());
          }
          abort();
          return false;
        }
        F_ILOG("ShaderStorage", "Compiled: \"%s\"", shaderName.c_str());
        m_fs.writeFile(dxilPath, faze::reinterpret_memView<const uint8_t>(faze::makeByteView(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize())));
        return true;
      }

      faze::MemView<uint8_t> shader(std::string shaderName, ShaderType type)
      {
        auto shaderPath = sourcePath + shaderName + "." + shaderFileType(type);
        auto dxilPath = compiledPath + shaderName + "." + shaderFileType(type) + ".dxil";

        if (!m_fs.fileExists(dxilPath))
        {
          //      F_ILOG("ShaderStorage", "First time compiling \"%s\"", shaderName.c_str());
          F_ASSERT(compileShader(shaderName, type), "ups");
        }
        if (m_fs.fileExists(dxilPath))
        {
          auto shaderInterfacePath = sourcePath + shaderName + ".if.hpp";

          auto shaderTime = m_fs.timeModified(shaderPath);
          auto shaderInterfaceTime = m_fs.timeModified(shaderInterfacePath);
          auto dxilTime = m_fs.timeModified(dxilPath);

          if (shaderTime > dxilTime || shaderInterfaceTime > dxilTime)
          {
            //        F_ILOG("ShaderStorage", "Spirv was old, compiling: \"%s\"", shaderName.c_str());
            F_ASSERT(compileShader(shaderName, type), "ups");
          }
        }
        F_ASSERT(m_fs.fileExists(dxilPath), "wtf???");
        auto shader = m_fs.readFile(dxilPath);
        return faze::makeByteView(shader.data(), shader.size());
      }

      ComPtr<ID3DBlob> rootSignature(std::string shaderName, ShaderType type)
      {
        auto shd = shader(shaderName, type);

        ComPtr<ID3DBlob> rootBlob;
        // extract root
        D3DGetBlobPart(shd.data(), shd.size(), D3D_BLOB_ROOT_SIGNATURE, 0, rootBlob.GetAddressOf());
        return rootBlob;
      }
    };
  }
}
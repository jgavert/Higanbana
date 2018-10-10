#pragma once
#if defined(FAZE_PLATFORM_WINDOWS)
#include "core/src/filesystem/filesystem.hpp"
#include "core/src/math/math.hpp"
#include "core/src/global_debug.hpp"
#include "faze/src/new_gfx/common/descriptor_layout.hpp"



#include <dxc/dxcapi.h>
#pragma warning( push )
#pragma warning( disable : 4100)
#include <dxc/Support/microcom.h>
#pragma warning( pop )

#include <fstream>
#include <string>

#include <wrl.h>
#include <Objbase.h>

template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;


class DXCIncludeHandler2 : public IDxcIncludeHandler
{
public:

  DXCIncludeHandler2(faze::FileSystem& fs, std::string sourcePath, std::string rootSignature, ComPtr<IDxcLibrary> lib, std::function<void(std::string)> func)
    : m_fs(fs), m_sourcePath(sourcePath), m_rootSignatureFile(rootSignature), m_lib(lib), m_fileIncluded(func)
  {
    if (m_rootSignatureFile.empty())
    {
      m_rootSignatureFile = "\n\n";
    }
  }

  DXC_MICROCOM_ADDREF_RELEASE_IMPL(m_dwRef)
    virtual ~DXCIncludeHandler2() {}

  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) {
    return DoBasicQueryInterface<::IDxcIncludeHandler>(this, riid, ppvObject);
  }

  HRESULT STDMETHODCALLTYPE LoadSource(
    _In_ LPCWSTR pFilename,                                   // Candidate filename.
    _COM_Outptr_result_maybenull_ IDxcBlob **ppIncludeSource  // Resultant source object for included file, nullptr if not found.
  ) override
  {
    //ppIncludeSource = nullptr;
    std::string filename = ws2s(pFilename);
    if (!filename.empty())
      filename = filename.substr(2);

    std::string finalPath;
    finalPath = m_sourcePath + filename;
    if (filename.compare("rootSig.h") == 0)
    {
      F_LOG("Shader include wanted RootSignature!!\n");

      ComPtr<IDxcBlobEncoding> asd;
      auto hr = m_lib->CreateBlobWithEncodingOnHeapCopy(m_rootSignatureFile.data(), static_cast<UINT32>(m_rootSignatureFile.size()), CP_ACP, asd.ReleaseAndGetAddressOf());

      if (SUCCEEDED(hr))
      {
        *ppIncludeSource = asd.Detach();
      }
      return hr;
    }
    else
    {
      F_ASSERT(m_fs.fileExists(finalPath), "Shader file doesn't exists in path %s\n", finalPath.c_str());
    }

    if (m_fileIncluded)
    {
      m_fileIncluded(finalPath);
    }

    auto shader = m_fs.viewToFile(finalPath);
    ComPtr<IDxcBlobEncoding> asd;
    auto hr = m_lib->CreateBlobWithEncodingOnHeapCopy(shader.data(), static_cast<uint32_t>(shader.size()), CP_ACP, asd.ReleaseAndGetAddressOf());

    if (SUCCEEDED(hr))
    {
      *ppIncludeSource = asd.Detach();
    }
    else
    {
      F_LOG("oh no\n");
    }

    return hr;
  }
private:
  DXC_MICROCOM_REF_FIELD(m_dwRef)
  faze::FileSystem& m_fs;
  std::string m_sourcePath;
  std::string m_rootSignatureFile;
  std::function<void(std::string)> m_fileIncluded;
  ComPtr<IDxcLibrary> m_lib;
};

namespace faze
{
  namespace backend
  {
    enum class ShaderType
    {
      Vertex,
      Pixel,
      Compute,
      Geometry,
      TessControl,
      TessEvaluation
    };

    enum class ShaderBinaryType
    {
      SPIRV,
      DXIL
    };

    struct ShaderCreateInfo
    {
      struct Descriptor
      {
        std::string shaderName = "";
        ShaderType type = ShaderType::Compute;
        std::vector<std::string> definitions = {};
        uint3 tgs = uint3(1,1,1);
        std::string rootSignature = "";
        int srvOffset = 0;
        int uavOffset = 0;
        int samplerOffset = 0;
      } desc;

      ShaderCreateInfo(std::string shaderName, ShaderType type)
      {
        desc.shaderName = shaderName;
        desc.type = type;
      }
      ShaderCreateInfo& setRootSignature(std::string value)
      {
        desc.rootSignature = value;
        return *this;
      }
      ShaderCreateInfo& setLayoutOffsets(DescriptorLayout layout)
      {
        desc.srvOffset = 1;
        desc.uavOffset = desc.srvOffset + layout.srvCount();
        desc.samplerOffset = desc.uavOffset + layout.uavCount();
        return *this;
      }
      ShaderCreateInfo& setSRVOffset(int value)
      {
        desc.srvOffset = value;
        return *this;
      }
      ShaderCreateInfo& setUAVOffset(int value)
      {
        desc.uavOffset = value;
        return *this;
      }
      ShaderCreateInfo& setSamplerOffset(int value)
      {
        desc.samplerOffset = value;
        return *this;
      }
      ShaderCreateInfo& setDefinitions(std::vector<std::string> value)
      {
        desc.definitions = value;
        return *this;
      }
      ShaderCreateInfo& setComputeGroups(uint3 value)
      {
        desc.tgs = value;
        return *this;
      }
    };

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

    class DXCompiler : public ShaderCompiler
    {
      faze::FileSystem& m_fs;
      std::string m_sourcePath;

      const wchar_t* DXCompiler::shaderFeatureDXC(ShaderType type)
      {
        switch (type)
        {
        case ShaderType::Vertex:
          return L"vs_6_0";
        case ShaderType::Pixel:
          return L"ps_6_0";
        case ShaderType::Compute:
          return L"cs_6_0";
        case ShaderType::Geometry:
          return L"gs_6_0";
        case ShaderType::TessControl: // hs_6_0 ??
        case ShaderType::TessEvaluation: // ds_6_0 ??
        default:
          F_ASSERT(false, "Unknown ShaderType");
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
  }
}

#endif
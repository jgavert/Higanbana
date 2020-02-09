#include "higanbana/graphics/shaders/DXCompiler.hpp"
#if defined(HIGANBANA_PLATFORM_WINDOWS)
#include <wrl.h>
#include <Objbase.h>
#include <dxc/dxcapi.h>
#include <dxc/Support/microcom.h>
template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

class DXCIncludeHandler2 : public IDxcIncludeHandler
{
public:
  DXCIncludeHandler2(higanbana::FileSystem& fs, std::string sourcePath, std::string rootSignature, ComPtr<IDxcLibrary> lib, std::function<void(std::string)> func): m_fs(fs)
    , m_sourcePath(sourcePath)
    , m_rootSignatureFile(rootSignature)
    , m_lib(lib)
    , m_fileIncluded(func)
  {
    if (m_rootSignatureFile.empty())
    {
      m_rootSignatureFile = "\n\n";
    }
  }

  DXC_MICROCOM_ADDREF_RELEASE_IMPL(m_dwRef)
    virtual ~DXCIncludeHandler2() {}

  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override {
    return DoBasicQueryInterface<::IDxcIncludeHandler>(this, riid, ppvObject);
  }

  int findFirstNonSpecialCharacter(const std::string& filename) noexcept
  {
    for (int i = 0; i < static_cast<int>(filename.size()); i++) {
      char problem = filename[i];
      if (problem == '/' || problem == '\\' || problem == '.')
        continue;
      return i;
    }
    return filename.size();
  }

  HRESULT STDMETHODCALLTYPE LoadSource(
    _In_ LPCWSTR pFilename,                                   // Candidate filename.
    _COM_Outptr_result_maybenull_ IDxcBlob **ppIncludeSource  // Resultant source object for included file, nullptr if not found.
  ) override
  {
    //ppIncludeSource = nullptr;
    std::string filename = ws2s(pFilename);
    if (!filename.empty())
    {
      auto substrStart = findFirstNonSpecialCharacter(filename);
      filename = filename.substr(substrStart);
    }

    std::string finalPath;
    finalPath = m_sourcePath + filename;
    if (filename.compare("rootSig.h") == 0)
    {
      HIGAN_LOG("Shader include wanted RootSignature!!\n");

      ComPtr<IDxcBlobEncoding> asd;
      auto hr = m_lib->CreateBlobWithEncodingFromPinned(m_rootSignatureFile.data(), static_cast<UINT32>(m_rootSignatureFile.size()), CP_ACP, asd.ReleaseAndGetAddressOf());

      if (SUCCEEDED(hr))
      {
        *ppIncludeSource = asd.Detach();
      }
      return hr;
    }
    else
    {
      HIGAN_ASSERT(m_fs.fileExists(finalPath), "Shader file doesn't exists in path %s\n", finalPath.c_str());
    }

    if (m_fileIncluded)
    {
      m_fileIncluded(finalPath);
    }

    auto shader = m_fs.viewToFile(finalPath);
    ComPtr<IDxcBlobEncoding> asd;
    auto hr = m_lib->CreateBlobWithEncodingFromPinned(shader.data(), static_cast<uint32_t>(shader.size()), CP_ACP, asd.ReleaseAndGetAddressOf());

    if (SUCCEEDED(hr))
    {
      *ppIncludeSource = asd.Detach();
    }
    else
    {
      HIGAN_LOG("oh no\n");
    }

    return hr;
  }
private:
  higanbana::FileSystem& m_fs;
  std::string m_sourcePath;
  std::string m_rootSignatureFile;
  std::function<void(std::string)> m_fileIncluded;
  ComPtr<IDxcLibrary> m_lib;
  DXC_MICROCOM_REF_FIELD(m_dwRef)
};

namespace higanbana
{
  namespace backend
  {
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
      case ShaderType::Hull: // hull?
        return "tc";
      case ShaderType::Domain: // domain?
        return "te";
      case ShaderType::Amplification: // domain?
        return "as";
      case ShaderType::Mesh: // domain?
        return "ms";
      default:
        HIGAN_ASSERT(false, "Unknown ShaderType");
      }
      return "";
    }

    bool DXCompiler::compileShader(
      ShaderBinaryType binType
      , std::string shaderSourcePath
      , std::string shaderBinaryPath
      , ShaderCreateInfo info
      , std::function<void(std::string)> includeCallback)
    {
      auto d = info.desc;
      auto shaderPath = shaderSourcePath;
      auto dxilPath = shaderBinaryPath;

      HIGAN_ASSERT(m_fs.fileExists(shaderPath), "Shader file doesn't exists in path %s\n", shaderPath.c_str());
      auto view = m_fs.viewToFile(shaderPath);
      std::string text;
      text.resize(view.size());
      memcpy(reinterpret_cast<char*>(&text[0]), view.data(), view.size());

      auto TGS_X = s2ws(std::to_string(d.tgs.x));
      auto TGS_Y = s2ws(std::to_string(d.tgs.y));
      auto TGS_Z = s2ws(std::to_string(d.tgs.z));

      ComPtr<IDxcLibrary> pLibrary;
      ComPtr<IDxcBlobEncoding> pSource;
      DxcCreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary), (void **)&pLibrary);
      pLibrary->CreateBlobWithEncodingFromPinned(text.c_str(), static_cast<uint32_t>(text.size()), CP_UTF8, &pSource);

      ComPtr<IDxcIncludeHandler> dxcHandlerPtr(new DXCIncludeHandler2(m_fs, m_sourcePath, d.rootSignature, pLibrary, includeCallback));

      std::vector<LPCWSTR> ppArgs;
      std::vector<std::wstring> tempArgs;
      if (binType == ShaderBinaryType::SPIRV)
      {
        ppArgs.push_back(L"-spirv"); // enable spirv codegen
        ppArgs.push_back(L"-fspv-target-env=vulkan1.1");
        //ppArgs.push_back(L"-Oconfig=-O");
        //ppArgs.push_back(L"-Oconfig=--loop-unroll,--scalar-replacement=300,--eliminate-dead-code-aggressive");
        //ppArgs.push_back(L"-0d");
      }
      else
      {
        ppArgs.push_back(L"/O3");
        //ppArgs.push_back(L"-Od"); // Disable optimizations. /Od implies /Gfp though output may not be identical to /Od /Gfp. 
                                // /Gfp Prefer flow control constructs.
      }

      // other various settings
      ppArgs.push_back(L"/Zi"); // Enable debugging information.
      ppArgs.push_back(L"/WX"); // Treat warnings as errors.
      ppArgs.push_back(L"/Ges"); //Enable strict mode.
      ppArgs.push_back(L"/enable_unbounded_descriptor_tables"); //Enables unbounded descriptor tables.
      ppArgs.push_back(L"/all_resources_bound"); // Enable aggressive flattening in SM5.1+.
      ppArgs.push_back(L"/enable-16bit-types"); 
      /*
        /Zpc	Pack matrices in column-major order.
        /Zpr	Pack matrices in row-major order.
      */
      ppArgs.push_back(L"/Zpr"); // row-major matrices.

      ComPtr<IDxcCompiler2> pCompiler;
      DxcCreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler2), (void **)&pCompiler);

      std::vector<DxcDefine> defs;

      if (binType == ShaderBinaryType::DXIL)
      {
        defs.push_back(DxcDefine{ L"HIGANBANA_DX12", nullptr });
      }
      else
      {
        defs.push_back(DxcDefine{ L"HIGANBANA_VULKAN", nullptr});
      }

      if (d.type == ShaderType::Compute)
      {
        defs.push_back(DxcDefine{ L"HIGANBANA_THREADGROUP_X", TGS_X.c_str() });
        defs.push_back(DxcDefine{ L"HIGANBANA_THREADGROUP_Y", TGS_Y.c_str() });
        defs.push_back(DxcDefine{ L"HIGANBANA_THREADGROUP_Z", TGS_Z.c_str() });
      }

      std::vector<std::wstring> convertedDefs;

      for (auto&& it : d.definitions)
      {
        convertedDefs.push_back(s2ws(it));
      }

      for (auto&& it : convertedDefs)
      {
        defs.push_back(DxcDefine{ it.c_str(), nullptr });
      }

      ComPtr<IDxcOperationResult> pResult;

      std::wstring kek = s2ws(d.shaderName);

      pCompiler->Compile(
        pSource.Get(),                                      // program text
        kek.c_str(),                                        // file name, mostly for error messages
        L"main",                                            // entry point function
        shaderFeatureDXC(d.type),                             // target profile
        ppArgs.data(), static_cast<UINT32>(ppArgs.size()),  // compilation arguments
        defs.data(), static_cast<UINT32>(defs.size()),      // name/value defines and their count
        dxcHandlerPtr.Get(),                                // handler for #include directives
        &pResult);

      // TODO: PDB's for debugging https://blogs.msdn.microsoft.com/pix/using-automatic-shader-pdb-resolution-in-pix/

      HRESULT hr;

      pResult->GetStatus(&hr);
      ComPtr<IDxcBlobEncoding> errorblob;
      pResult->GetErrorBuffer(errorblob.GetAddressOf());

      if (FAILED(hr))
      {
        OutputDebugStringA("Error In \"");
        OutputDebugStringA(shaderPath.c_str());
        OutputDebugStringA("\": \n");

        const char *pStart = (const char *)errorblob->GetBufferPointer();
        std::string msg(pStart);

        OutputDebugStringA(msg.c_str());
        OutputDebugStringA("\n");

        HIGAN_ILOG("ShaderStorage", "Error In \"%s\":\n %s\n", shaderPath.c_str(), msg.c_str());
        //HIGAN_ASSERT(false, "Temp assertion for strange errors");
        return false;
      }
      HIGAN_ILOG("ShaderStorage", "Compiled: \"%s\"", d.shaderName.c_str());
      ComPtr<IDxcBlob> blob;
      pResult->GetResult(blob.ReleaseAndGetAddressOf());
      auto thingA = blob->GetBufferPointer();
      auto thingB = blob->GetBufferSize();
      auto viewToBlob = higanbana::reinterpret_memView<const uint8_t>(higanbana::makeByteView(thingA, thingB));
      m_fs.writeFile(dxilPath, viewToBlob);
      return true;

    }
  }
}
#endif
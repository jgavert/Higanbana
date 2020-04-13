#include "higanbana/graphics/shaders/DXCompiler.hpp"
#include <dxc/Support/WinIncludes.h>
#include <dxc/Support/Global.h>
#include <dxc/DxilContainer/DxilContainer.h>
#include <dxc/dxcapi.h>
#include <dxc/Support/microcom.h>

#include <higanbana/core/system/time.hpp>


class DXCIncludeHandler : public IDxcIncludeHandler {
  DXC_MICROCOM_REF_FIELD(m_dwRef)
public:
  DXCIncludeHandler(higanbana::FileSystem& fs, std::string sourcePath, std::string rootSignature, CComPtr<IDxcLibrary> lib, std::function<void(std::string)> func): m_fs(fs)
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

  void removeExtraSlashes(std::string& path) noexcept {
    auto iter = path.begin();
    auto lastChar = '0';
    while(iter != path.end()) {
      if (lastChar == '/' && *iter == '/') {
        iter = path.erase(iter);
        lastChar = '0';
        continue;
      }
      lastChar = *iter;
      iter++;
    }
  }

  HRESULT STDMETHODCALLTYPE LoadSource(
    _In_ LPCWSTR pFilename,                                   // Candidate filename.
    _COM_Outptr_  IDxcBlob **ppIncludeSource  // Resultant source object for included file, nullptr if not found.
  ) override
  {
    //ppIncludeSource = nullptr;
    std::string filename = ws2s(pFilename);
    if (!filename.empty())
    {
      auto substrStart = findFirstNonSpecialCharacter(filename);
      filename = filename.substr(substrStart);
      removeExtraSlashes(filename);
    }

    std::string finalPath;
    finalPath = m_sourcePath + filename;
    HIGAN_ASSERT(m_fs.fileExists(finalPath), "Shader file doesn't exists in path %s\n", finalPath.c_str());

    if (m_fileIncluded)
    {
      m_fileIncluded(finalPath);
    }

    auto shader = m_fs.viewToFile(finalPath);
    CComPtr<IDxcBlobEncoding> asd;
    auto hr = m_lib->CreateBlobWithEncodingFromPinned(shader.data(), static_cast<uint32_t>(shader.size()), CP_ACP, &asd);

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
  CComPtr<IDxcLibrary> m_lib;
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
      Timer time;
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

      CComPtr<IDxcLibrary> pLibrary;
      CComPtr<IDxcBlobEncoding> pSource;
      DxcCreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary), (void **)&pLibrary);
      pLibrary->CreateBlobWithEncodingFromPinned(text.c_str(), static_cast<uint32_t>(text.size()), CP_UTF8, &pSource);

      CComPtr<IDxcIncludeHandler> dxcHandlerPtr(new DXCIncludeHandler(m_fs, m_sourcePath, d.rootSignature, pLibrary, includeCallback));

      std::vector<LPCWSTR> ppArgs;
      std::vector<std::wstring> tempArgs;

      if (binType == ShaderBinaryType::SPIRV)
      {
        ppArgs.push_back(L"-spirv"); // enable spirv codegen
        ppArgs.push_back(L"-fspv-target-env=vulkan1.1");
        ppArgs.push_back(L"-fvk-use-dx-layout");
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
      ppArgs.push_back(L"/all_resources_bound"); // Enable aggressive flattening in SM5.1+.
      ppArgs.push_back(L"/enable-16bit-types"); // needs _6_2 shaders minimum
      /*
        /Zpc	Pack matrices in column-major order.
        /Zpr	Pack matrices in row-major order.
      */
      ppArgs.push_back(L"/Zpr"); // row-major matrices.

      CComPtr<IDxcCompiler3> pCompiler;
      DxcCreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler3), (void **)&pCompiler);

      if (binType == ShaderBinaryType::DXIL)
      {
        ppArgs.push_back(L"/DHIGANBANA_DX12");
        //defs.push_back(DxcDefine{ L"HIGANBANA_DX12", nullptr });
      }
      else
      {
        ppArgs.push_back(L"/DHIGANBANA_VULKAN");
        //defs.push_back(DxcDefine{ L"HIGANBANA_VULKAN", nullptr});
      }

      std::wstring tx, ty, tz;
      if (d.type == ShaderType::Compute)
      {
        tx = L"/DHIGANBANA_THREADGROUP_X=";
        tx += TGS_X;
        ty = L"/DHIGANBANA_THREADGROUP_Y=";
        ty += TGS_Y;
        tz = L"/DHIGANBANA_THREADGROUP_Z=";
        tz += TGS_Z;
        ppArgs.push_back(tx.c_str());
        ppArgs.push_back(ty.c_str());
        ppArgs.push_back(tz.c_str());
        //defs.push_back(DxcDefine{ L"HIGANBANA_THREADGROUP_X", TGS_X.c_str() });
        //defs.push_back(DxcDefine{ L"HIGANBANA_THREADGROUP_Y", TGS_Y.c_str() });
        //defs.push_back(DxcDefine{ L"HIGANBANA_THREADGROUP_Z", TGS_Z.c_str() });
      }

      std::vector<std::wstring> convertedDefs;

      for (auto&& it : d.definitions)
      {
        std::wstring wtr = L"/D";
        wtr += s2ws(it);
        convertedDefs.push_back(wtr);
      }

      for (auto&& it : convertedDefs)
      {
        //defs.push_back(DxcDefine{ it.c_str(), nullptr });
        ppArgs.push_back(it.c_str());
      }

      /*
      CComPtr<DxcResult> pResult = DxcResult::Alloc(m_pMalloc);
      pResult->SetEncoding(opts.DefaultTextCodePage);

      CComPtr<IDxcResult> pImplResult;
      */

      CComPtr<IDxcOperationResult> pResult;

      ppArgs.push_back(L"/Emain");
      ppArgs.push_back(shaderFeatureDXC(d.type));

      DxcBuffer srcBuffer = { pSource->GetBufferPointer(), pSource->GetBufferSize(), 0 };

      std::wstring kek = s2ws(d.shaderName);
      pCompiler->Compile(
        &srcBuffer,                                      // program text
        ppArgs.data(), static_cast<UINT32>(ppArgs.size()),  // compilation arguments
        dxcHandlerPtr,                                // handler for #include directives
        IID_PPV_ARGS(&pResult));

      // TODO: PDB's for debugging https://blogs.msdn.microsoft.com/pix/using-automatic-shader-pdb-resolution-in-pix/

      HRESULT hr;

      pResult->GetStatus(&hr);
      CComPtr<IDxcBlobEncoding> errorblob;
      pResult->GetErrorBuffer(&errorblob);

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
      auto timingFinish = float(time.timeFromLastReset());
      HIGAN_ILOG("ShaderStorage", "Compiled: \"%s\" in %.2fms", dxilPath.c_str(), timingFinish / 1000000.f);
      CComPtr<IDxcBlob> blob;
      pResult->GetResult(&blob);
      auto thingA = blob->GetBufferPointer();
      auto thingB = blob->GetBufferSize();
      auto viewToBlob = higanbana::reinterpret_memView<const uint8_t>(higanbana::makeByteView(thingA, thingB));
      m_fs.writeFile(dxilPath, viewToBlob);

      // Extract debug blob if present
      /*
      CComHeapPtr<wchar_t> pDebugNameOnComHeap;
      CComPtr<IDxcBlob> pDebugBlob;
      if (SUCCEEDED(hr)) {
        CComPtr<IDxcBlobUtf16> pDebugName;
        hr = pResult->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&pDebugBlob), &pDebugName);
        if (SUCCEEDED(hr) && ppDebugBlobName && pDebugName) {
          if (!pDebugNameOnComHeap.AllocateBytes(pDebugName->GetBufferSize()))
            return E_OUTOFMEMORY;
          memcpy(pDebugNameOnComHeap.m_pData, pDebugName->GetBufferPointer(), pDebugName->GetBufferSize());
        }
      }*/
      return true;

    }
  }
}
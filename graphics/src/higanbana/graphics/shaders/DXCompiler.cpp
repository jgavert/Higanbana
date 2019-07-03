#include "higanbana/graphics/shaders/DXCompiler.hpp"
#if defined(HIGANBANA_PLATFORM_WINDOWS)
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
      case ShaderType::TessControl: // hull?
        return "tc";
      case ShaderType::TessEvaluation: // domain?
        return "te";
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
      pLibrary->CreateBlobWithEncodingOnHeapCopy(text.c_str(), static_cast<uint32_t>(text.size()), CP_UTF8, &pSource);

      ComPtr<IDxcIncludeHandler> dxcHandlerPtr(new DXCIncludeHandler2(m_fs, m_sourcePath, d.rootSignature, pLibrary, includeCallback));

      std::vector<LPCWSTR> ppArgs;
      std::vector<std::wstring> tempArgs;
      if (binType == ShaderBinaryType::SPIRV)
      {
        ppArgs.push_back(L"-spirv"); // enable spirv codegen
        ppArgs.push_back(L"-fspv-target-env=vulkan1.1");
      }


      // other various settings
      ppArgs.push_back(L"/Zi"); // Enable debugging information.
      ppArgs.push_back(L"/WX"); // Treat warnings as errors.
      ppArgs.push_back(L"/Od"); // Disable optimizations. /Od implies /Gfp though output may not be identical to /Od /Gfp. 
                                // /Gfp Prefer flow control constructs.
      ppArgs.push_back(L"/Ges"); //Enable strict mode.
      ppArgs.push_back(L"/enable_unbounded_descriptor_tables"); //Enables unbounded descriptor tables.
      ppArgs.push_back(L"/all_resources_bound"); // Enable aggressive flattening in SM5.1+.
      ppArgs.push_back(L"-enable-16bit-types"); 
      /*
        /Zpc	Pack matrices in column-major order.
        /Zpr	Pack matrices in row-major order.
      */

      ComPtr<IDxcCompiler> pCompiler;
      DxcCreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler), (void **)&pCompiler);

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
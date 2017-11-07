#pragma once

#include "core/src/filesystem/filesystem.hpp"
#include "core/src/global_debug.hpp"

#include "faze/src/new_gfx/dx12/dx12.hpp"

#include <fstream>
#include <string>

/*
IDxcIncludeHandler : public IUnknown {
virtual HRESULT STDMETHODCALLTYPE LoadSource(
_In_ LPCWSTR pFilename,                                   // Candidate filename.
_COM_Outptr_result_maybenull_ IDxcBlob **ppIncludeSource  // Resultant source object for included file, nullptr if not found.
) = 0;
};
*/

class DXCIncludeHandler : public IDxcIncludeHandler
{
public:
  DXCIncludeHandler(faze::FileSystem& fs, std::string sourcePath, ComPtr<IDxcLibrary> lib) : m_fs(fs), m_sourcePath(sourcePath), m_lib(lib) {}
  DXC_MICROCOM_ADDREF_RELEASE_IMPL(m_dwRef)
    virtual ~DXCIncludeHandler() {}

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

    F_LOG("DXCIncludeHandler %s\n", filename.c_str());

    std::string finalPath;
    if (filename.size() > 15 && filename.compare(filename.size() - 15, 15, "definitions.hpp") == 0)
    {
      finalPath = "/../" + filename;
      if (!m_fs.fileExists(finalPath))
        m_fs.loadDirectoryContentsRecursive("/../app/graphics/");
    }
    else
    {
      finalPath = m_sourcePath + filename;
      // m_sourcePath + 
    }
    F_ASSERT(m_fs.fileExists(finalPath), "Shader file doesn't exists in path %s\n", finalPath.c_str());

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
  ComPtr<IDxcLibrary> m_lib;
};

class CShaderInclude : public ID3DInclude {
public:
  CShaderInclude(faze::FileSystem& fs, std::string sourcePath) : m_fs(fs), m_sourcePath(sourcePath) {}

  HRESULT __stdcall Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID /*pParentData*/, LPCVOID *ppData, UINT *pBytes)
  {
    std::string filename(pFileName);
    std::string finalPath;
    if (filename.size() > 15 && filename.compare(filename.size() - 15, 15, "definitions.hpp") == 0)
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
      std::string sourceCopyPath;
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

      const wchar_t* shaderFeatureDXC(ShaderType type)
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
        case ShaderType::TessControl:
        case ShaderType::TessEvaluation:
        default:
          F_ASSERT(false, "Unknown ShaderType");
        }
        return L"";
      }

    public:
      DX12ShaderStorage(FileSystem& fs, std::string shaderPath, std::string binaryPath)
        : m_fs(fs)
        , sourcePath("/" + shaderPath + "/")
        , compiledPath("/" + binaryPath + "/")
        , sourceCopyPath(compiledPath + "source/")
      {
        m_fs.loadDirectoryContentsRecursive(sourcePath);
        // we could compile all shaders that don't have spv ahead of time
        // requires support from filesystem
        vector<std::pair<std::string, MemView<const uint8_t>>> filesToCopy;
        m_fs.getFilesWithinDir(sourcePath, [&](std::string& path, MemView<const uint8_t> data)
        {
          //F_LOG("shader source found %s, would like to copy to %s\n", path.c_str(), sourceCopyPath.c_str());
          filesToCopy.emplace_back(std::make_pair(path, data));
        });
        m_fs.loadDirectoryContentsRecursive("/../app/graphics/");
        m_fs.getFilesWithinDir("/../app/graphics/", [&](std::string& path, MemView<const uint8_t> data)
        {
          if (path.compare("definitions.hpp") == 0)
          {
            //F_LOG("shader source found %s, would like to copy to %s\n", path.c_str(), sourceCopyPath.c_str());
            filesToCopy.emplace_back(std::make_pair(path, data));
          }
        });
        for (auto&& file : filesToCopy)
        {
          auto finalPath = sourceCopyPath + file.first;
          m_fs.writeFile(finalPath, file.second);
        }
        m_fs.loadDirectoryContentsRecursive(sourceCopyPath);
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
        //printf("%s\n", text.data());
        // we got shader in "text"

        ComPtr<IDxcLibrary> pLibrary;
        ComPtr<IDxcBlobEncoding> pSource;
        DxcCreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary), (void **)&pLibrary);
        pLibrary->CreateBlobWithEncodingOnHeapCopy(text.c_str(), static_cast<uint32_t>(text.size()), CP_UTF8, &pSource);

        //DXCIncludeHandler dxcHandler(m_fs, sourcePath, pLibrary);

        ComPtr<IDxcIncludeHandler> dxcHandlerPtr(new DXCIncludeHandler(m_fs, sourcePath, pLibrary));

        LPCWSTR ppArgs[] = { L"/Zi" }; // debug info
        ComPtr<IDxcCompiler> pCompiler;
        DxcCreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler), (void **)&pCompiler);
        DxcDefine defs[] = { DxcDefine{ L"FAZE_DX12", nullptr } };
        ComPtr<IDxcOperationResult> pResult;

        std::wstring kek = s2ws(shaderName);

        pCompiler->Compile(
          pSource.Get(),          // program text
          kek.c_str(),   // file name, mostly for error messages
          L"main",          // entry point function
          shaderFeatureDXC(type),        // target profile
          ppArgs,           // compilation arguments
          _countof(ppArgs), // number of compilation arguments
          defs, _countof(defs),       // name/value defines and their count
          dxcHandlerPtr.Get(),          // handler for #include directives
          &pResult);

        HRESULT hr;

        pResult->GetStatus(&hr);

        if (FAILED(hr))
        {
          ComPtr<IDxcBlobEncoding> blob;
          pResult->GetErrorBuffer(blob.GetAddressOf());

          OutputDebugStringA("Error In \"");
          OutputDebugStringA(shaderPath.c_str());
          OutputDebugStringA("\": \n");
          OutputDebugStringA((char*)blob->GetBufferPointer());

          F_ILOG("ShaderStorage", "Error In \"%s\":\n %s\n", shaderPath.c_str(), (char*)blob->GetBufferPointer());
          return false;
        }
        F_ILOG("ShaderStorage", "Compiled: \"%s\"", shaderName.c_str());
        ComPtr<IDxcBlob> blob;
        pResult->GetResult(blob.ReleaseAndGetAddressOf());
        auto thingA = blob->GetBufferPointer();
        auto thingB = blob->GetBufferSize();
        auto viewToBlob = faze::reinterpret_memView<const uint8_t>(faze::makeByteView(thingA, thingB));
        m_fs.writeFile(dxilPath, viewToBlob);
        return true;
        /*
        CShaderInclude include(m_fs, sourcePath);
        auto p = shaderFeature(type);
        UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS | /*D3DCOMPILE_WARNINGS_ARE_ERRORS |*/ D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES | D3DCOMPILE_ALL_RESOURCES_BOUND;
#if !defined(RELEASE)
        //compileFlags |= D3DCOMPILE_DEBUG;
#endif
#if !defined(DEBUG)
        //compileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
        /*
        ComPtr<ID3DBlob> shaderBlob;
        ComPtr<ID3DBlob> errorMsg;

        D3D_SHADER_MACRO macros[] =
        {
          D3D_SHADER_MACRO{"FAZE_DX12", nullptr },
          D3D_SHADER_MACRO{nullptr, nullptr }
        };

        hr = D3DCompile(text.data(), text.size(), shaderName.c_str(), macros, &include, "main", p, compileFlags, 0, shaderBlob.GetAddressOf(), errorMsg.GetAddressOf());
        // https://msdn.microsoft.com/en-us/library/dn859356(v=vs.85).aspx
        if (FAILED(hr))
        {
          if (errorMsg.Get())
          {
            /*
                OutputDebugStringA("Error In \"");
                OutputDebugStringA(shaderPath.c_str());
                OutputDebugStringA("\": \n");
                OutputDebugStringA((char*)errorMsg->GetBufferPointer());*/

                //F_ILOG("ShaderStorage", "Error In \"%s\":\n %s\n", shaderPath.c_str(), (char*)errorMsg->GetBufferPointer());
              //}
              //abort();
              //return false;
            //}

            //F_ILOG("ShaderStorage", "Compiled: \"%s\"", shaderName.c_str());
            //m_fs.writeFile(dxilPath, faze::reinterpret_memView<const uint8_t>(faze::makeByteView(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize())));
            //return true;
      }

      faze::MemoryBlob shader(const std::string& shaderName, ShaderType type)
      {
        auto shaderPath = sourcePath + shaderName + "." + shaderFileType(type);
        auto dxilPath = compiledPath + shaderName + "." + shaderFileType(type) + ".dxil";

        if (!m_fs.fileExists(dxilPath))
        {
          //      F_ILOG("ShaderStorage", "First time compiling \"%s\"", shaderName.c_str());
          F_ASSERT(compileShader(shaderName, type), "ups");
        }
        if (m_fs.fileExists(dxilPath) && m_fs.fileExists(shaderPath))
        {
          auto shaderInterfacePath = sourcePath + shaderName + ".if.hpp";

          auto shaderTime = m_fs.timeModified(shaderPath);
          auto shaderInterfaceTime = m_fs.timeModified(shaderInterfacePath);
          auto dxilTime = m_fs.timeModified(dxilPath);

          if (shaderTime > dxilTime || shaderInterfaceTime > dxilTime)
          {
            //        F_ILOG("ShaderStorage", "Spirv was old, compiling: \"%s\"", shaderName.c_str());
            bool result = compileShader(shaderName, type);
            if (!result)
            {
              F_ILOG("DX12", "Shader compile failed.\n");
            }
          }
        }
        F_ASSERT(m_fs.fileExists(dxilPath), "wtf???");
        auto shader = m_fs.readFile(dxilPath);
        return shader;
      }

      ComPtr<ID3DBlob> rootSignature(std::string shaderName, ShaderType type)
      {
        auto shd = shader(shaderName, type);

        ComPtr<ID3DBlob> rootBlob;
        // extract root
        D3DGetBlobPart(shd.data(), shd.size(), D3D_BLOB_ROOT_SIGNATURE, 0, rootBlob.GetAddressOf());
        return rootBlob;
      }

      WatchFile watch(std::string shaderName, ShaderType type)
      {
        auto shd = sourcePath + shaderName + "." + shaderFileType(type);
        if (m_fs.fileExists(shd))
        {
          return m_fs.watchFile(shd);
        }
        return WatchFile{};
      }
    };
  }
}
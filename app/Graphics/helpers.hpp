#pragma once
#include "ComPtr.hpp"
#include "Descriptors/Descriptors.hpp"
#include "binding.hpp"
#include <string>
#include <fstream>
#include <d3d12.h>
#include <D3Dcompiler.h>

// RESOURCE CREATION HELPERS

template <typename ...Args>
void fillData(D3D12_RESOURCE_DESC&, D3D12_HEAP_PROPERTIES&)
{}

template <typename ...Args>
void fillData(D3D12_RESOURCE_DESC& desc, D3D12_HEAP_PROPERTIES& heapProp,
  ArraySize size, Args... args)
{
  desc.DepthOrArraySize = size.size;
  fillData(desc, heapProp, std::forward<Args>(args)...);
}

template <typename ...Args>
void fillData(D3D12_RESOURCE_DESC& desc, D3D12_HEAP_PROPERTIES& heapProp,
  Dimension dim, Args... args)
{
  desc.Width *= dim.width;
  desc.Height *= dim.height;
  fillData(desc, heapProp, std::forward<Args>(args)...);
}

template <typename T, typename ...Args>
void fillData(D3D12_RESOURCE_DESC& desc, D3D12_HEAP_PROPERTIES&heapProp,
  Format<T> format, Args... args)
{
  desc.Format = FormatToDXGIFormat[format.format];
  if (FormatType::Unknown == format.format) // ONLY FOR BUFFERS !?!?!?!?
  {
    desc.DepthOrArraySize = format.stride;
  }
  fillData(desc, heapProp, std::forward<Args>(args)...);
}

template <typename ...Args>
void fillData(D3D12_RESOURCE_DESC& desc, D3D12_HEAP_PROPERTIES& heapProp,
  GpuShared shared, Args... args)
{
  if (shared.shared)
  {
    heapProp.VisibleNodeMask = 1;
    heapProp.CreationNodeMask = 1;
    desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER;
  }
  fillData(desc, heapProp, std::forward<Args>(args)...);
}

template <typename ...Args>
void fillData(D3D12_RESOURCE_DESC& desc, D3D12_HEAP_PROPERTIES& heapProp,
  ResUsage type, Args... args)
{
  switch (type)
  {
  case ResUsage::Upload:
    heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    break;
  case ResUsage::Gpu:
    heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
    break;
  case ResUsage::Readback:
    heapProp.Type = D3D12_HEAP_TYPE_READBACK;
    heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    break;
  default:
    heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
    break;
  }
  fillData(desc, heapProp, std::forward<Args>(args)...);
}

template <typename ...Args>
void fillData(D3D12_RESOURCE_DESC& desc, D3D12_HEAP_PROPERTIES& heapProp,
  Multisampling sampling, Args... args)
{
  desc.SampleDesc.Count = sampling.count;
  desc.SampleDesc.Quality = sampling.quality;
  fillData(desc, heapProp, std::forward<Args>(args)...);
}

template <typename ...Args>
void fillData(D3D12_RESOURCE_DESC& desc, D3D12_HEAP_PROPERTIES& heapProp,
  MipLevel mip, Args... args)
{
  desc.MipLevels = mip.level;
  fillData(desc, heapProp, std::forward<Args>(args)...);
}

class stringutils
{
public:
  static std::wstring s2ws(const std::string& s)
  {
    int len;
    int slength = (int)s.length() + 1;
    len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
    wchar_t* buf = new wchar_t[len];
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
    std::wstring r(buf);
    delete[] buf;
    return r;
  }
};

class CShaderInclude : public ID3DInclude {
public:
  CShaderInclude(const char* shaderDir, const char* systemDir) : m_ShaderDir(shaderDir), m_SystemDir(systemDir) {}

  HRESULT __stdcall Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID /*pParentData*/, LPCVOID *ppData, UINT *pBytes) {
    try {
      std::string finalPath;
      switch (IncludeType) {
      case D3D_INCLUDE_LOCAL:
        finalPath =  pFileName;
          break;
      case D3D_INCLUDE_SYSTEM:
        finalPath = pFileName;
          break;
      default:
        assert(0);
      }
      std::ifstream includeFile(finalPath.c_str(), std::ios::in | std::ios::binary | std::ios::ate);

      if (includeFile.is_open()) {
        long long fileSize = includeFile.tellg();
        char* buf = new char[fileSize];
        includeFile.seekg(0, std::ios::beg);
        includeFile.read(buf, fileSize);
        includeFile.close();
        *ppData = buf;
        *pBytes = static_cast<UINT>(fileSize);
      }
      else {
        return E_FAIL;
      }
      return S_OK;
    }
    catch (std::exception& /*e*/) {
      return E_FAIL;
    }
  }
  HRESULT __stdcall Close(LPCVOID pData) {
    char* buf = (char*)pData;
    delete[] buf;
    return S_OK;
  }
private:
  std::string m_ShaderDir;
  std::string m_SystemDir;
};


enum class ShaderType
{
  Vertex,
  Pixel,
  Geometry,
  Hull,
  Domain,
  Compute
};

class shaderUtils
{
public:

private:
  static std::pair<std::string, std::string> getShaderParams(ShaderType type)
  {
    switch (type)
    {
    case ShaderType::Vertex:
      return std::pair<std::string, std::string>("VSMain", "vs_5_1");
    case ShaderType::Pixel:
      return std::pair<std::string, std::string>("PSMain", "ps_5_1");
    case ShaderType::Geometry:
      return std::pair<std::string, std::string>("GSMain", "gs_5_1");
    case ShaderType::Hull:
      return std::pair<std::string, std::string>("HSMain", "hs_5_1");
    case ShaderType::Domain:
      return std::pair<std::string, std::string>("DSMain", "ds_5_1");
    case ShaderType::Compute:
      return std::pair<std::string, std::string>("CSMain", "cs_5_1");
    default:
      break;
    }
    return std::pair<std::string, std::string>("CSMain", "cs_5_1");
  }
public:

  static void getShaderInfo(ComPtr<ID3D12Device>& dev, ShaderType type, std::wstring path, ShaderInterface& root, ComPtr<ID3DBlob>& shaderBlob)
  {
    ComPtr<ID3DBlob> errorMsg;

    auto woot = path;
    CShaderInclude include("", "");
    auto p = getShaderParams(type);
	UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS;
#if !defined(RELEASE)
	compileFlags |= D3DCOMPILE_DEBUG;
#endif
#if !defined(DEBUG)
	compileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
    HRESULT hr = D3DCompileFromFile(woot.c_str(), nullptr, &include, p.first.c_str(), p.second.c_str(), compileFlags, 0, shaderBlob.addr(), errorMsg.addr());
    // https://msdn.microsoft.com/en-us/library/dn859356(v=vs.85).aspx
    if (FAILED(hr))
    {
      if (errorMsg.get())
      {
        OutputDebugStringA((char*)errorMsg->GetBufferPointer());
        errorMsg->Release();
      }
      abort();
    }
    {
      ComPtr<ID3D12RootSignatureDeserializer> asd;
      hr = D3D12CreateRootSignatureDeserializer(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), __uuidof(ID3D12RootSignatureDeserializer), reinterpret_cast<void**>(asd.addr()));
      if (FAILED(hr))
      {
        abort();
      }

      const D3D12_ROOT_SIGNATURE_DESC* woot2 = asd->GetRootSignatureDesc();
      // have valid shaderInterface?
      if (root.valid())
      {
        // see if this shader differs, and if so, abort. not ok.
        if (!root.isCopyOf(*woot2))
        {
          abort();
        }
      }
      else
      {
        // create new shaderinterface
        ComPtr<ID3D12RootSignature> rootSig;
        hr = dev->CreateRootSignature(
          1, shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(),
          __uuidof(ID3D12RootSignature), reinterpret_cast<void**>(rootSig.addr()));
        if (FAILED(hr))
        {
          abort();
        }
        root = ShaderInterface(rootSig, *woot2);
      }
    }
  }

  static std::vector<std::tuple<UINT, Binding_::RootType, unsigned int>> getRootDescriptorReflection(const D3D12_ROOT_SIGNATURE_DESC* desc, size_t& cbv, size_t& srv, size_t& uav)
  {
    std::vector<std::tuple<UINT, Binding_::RootType, unsigned int>> bindingInput;
    cbv = 0, srv = 0, uav = 0;
	int rootConstant = 0;
    for (unsigned int i = 0; i < desc->NumParameters;++i)
    {
      auto it = desc->pParameters[i];
      switch (it.ParameterType)
      {

      case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
        //F_LOG("Descriptor tables not supported\n");
        break;
      case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
        //F_LOG("Root constants not yet supported\n");
        //F_LOG("32Bit constant Num32BitValues:%u RegisterSpace:%u ShaderRegister:%u\n", it.Constants.Num32BitValues, it.Constants.RegisterSpace, it.Constants.ShaderRegister);
		bindingInput.push_back(std::make_tuple(i, Binding_::RootType::Num32, rootConstant++));
        break;
      case D3D12_ROOT_PARAMETER_TYPE_CBV:
        bindingInput.push_back(std::make_tuple(i, Binding_::RootType::CBV, cbv++));
        if (it.Descriptor.ShaderRegister != cbv - 1)
        {
          //F_LOG(" WARNING! Shader registers don't match.\n");
        }
        //F_LOG("CBV RegisterSpace:%u ShaderRegister:%u\n", it.Descriptor.RegisterSpace, it.Descriptor.ShaderRegister);
        break;
      case D3D12_ROOT_PARAMETER_TYPE_SRV:
        bindingInput.push_back(std::make_tuple(i, Binding_::RootType::SRV, srv++));
        if (it.Descriptor.ShaderRegister != srv - 1)
        {
          //F_LOG(" WARNING! Shader registers don't match.\n");
        }
        //F_LOG("SRV RegisterSpace:%u ShaderRegister:%u\n", it.Descriptor.RegisterSpace, it.Descriptor.ShaderRegister);
        break;
      case D3D12_ROOT_PARAMETER_TYPE_UAV:
        bindingInput.push_back(std::make_tuple(i, Binding_::RootType::UAV, uav++));
        if (it.Descriptor.ShaderRegister != uav - 1)
        {
          //F_LOG(" WARNING! Shader registers don't match.\n");
        }
        //F_LOG("UAV RegisterSpace:%u ShaderRegister:%u\n", it.Descriptor.RegisterSpace, it.Descriptor.ShaderRegister);
        break;
      default:
        break;
      }
    }
    return bindingInput;
  }
  
  static void getRootDescriptorReflection2(const D3D12_ROOT_SIGNATURE_DESC* desc, int& srvdesctable, int& uavdesctable)
  {
	  for (unsigned int i = 0; i < desc->NumParameters; ++i)
	  {
		  auto it = desc->pParameters[i];
		  switch (it.ParameterType)
		  {

		  case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
			  //F_LOG("Descriptor tables not supported\n");
		  {
			  auto num = it.DescriptorTable.NumDescriptorRanges;
			  if (num > 1)
			  {
				  continue;
			  }
			  for (int k = 0; k < num; ++k)
			  {
				  auto& it2 = it.DescriptorTable.pDescriptorRanges[k];
				  if (it2.RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SRV)
				  {
					  if (it2.NumDescriptors <= 128)
						  srvdesctable = it2.BaseShaderRegister;
				  }
				  else if (it2.RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_UAV)
				  {
					  if (it2.NumDescriptors <= 128)
						  uavdesctable = it2.BaseShaderRegister;
				  }
			  }
		  }
			  break;
		  default:
			  break;
		  }
	  }
  }
};

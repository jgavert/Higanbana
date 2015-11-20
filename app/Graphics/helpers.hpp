#pragma once
#include "ComPtr.hpp"
#include "Descriptors/Descriptors.hpp"
#include "binding.hpp"
#include <string>
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

class shaderUtils
{
public:
  static void getShaderInfo(ComPtr<ID3D12Device>& dev, std::wstring path, ComPtr<ID3D12RootSignature>& root, ComPtr<ID3DBlob>& shaderBlob)
  {
    ComPtr<ID3DBlob> errorMsg;

    auto woot = path;
    HRESULT hr = D3DCompileFromFile(woot.c_str(), nullptr, nullptr, "CSMain", "cs_5_1", 0, 0, shaderBlob.addr(), errorMsg.addr());
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
    if (!root.get())
    {
      ComPtr<ID3D12RootSignatureDeserializer> asd;
      hr = D3D12CreateRootSignatureDeserializer(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), __uuidof(ID3D12RootSignatureDeserializer), reinterpret_cast<void**>(asd.addr()));
      if (FAILED(hr))
      {
        abort();
      }
      ComPtr<ID3DBlob> blobSig;
      ComPtr<ID3DBlob> errorSig;
      const D3D12_ROOT_SIGNATURE_DESC* woot2 = asd->GetRootSignatureDesc();

      hr = D3D12SerializeRootSignature(woot2, D3D_ROOT_SIGNATURE_VERSION_1, blobSig.addr(), errorSig.addr());
      if (FAILED(hr))
      {
        abort();
      }

      hr = dev->CreateRootSignature(
        1, shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(),
        __uuidof(ID3D12RootSignature), reinterpret_cast<void**>(root.addr()));
      if (FAILED(hr))
      {
        abort();
      }
    }
  }

  static std::vector<std::tuple<UINT, Binding_::RootType, unsigned int>> getRootDescriptorReflection(const D3D12_ROOT_SIGNATURE_DESC* desc, size_t& cbv, size_t& srv, size_t& uav)
  {
    std::vector<std::tuple<UINT, Binding_::RootType, unsigned int>> bindingInput;
    cbv = 0, srv = 0, uav = 0;
    for (int i = 0; i < desc->NumParameters;++i)
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
};

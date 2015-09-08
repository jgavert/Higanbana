// GpuDevice.hpp I mean, how else would you name this? Provides compute and graphics.
// Need one for dx12 and vulkan, coexistance? I guess would be fun to compare on windows.
#pragma once

#include "Pipeline.hpp"
#include "PipelineDescriptor.hpp"
#include "CommandQueue.hpp"
#include "CommandList.hpp"
#include "SwapChain.hpp"
#include "Fence.hpp"
#include "Texture.hpp"
#include "Buffer.hpp"
#include "../Platform/Window.hpp"
#include "EvolRes.hpp"
#include "helpers.hpp"
#include "Descriptors/Descriptors.hpp"
#include <d3d12.h>
#include <D3Dcompiler.h>
#include <string>
class GpuDevice
{
private:
  friend class test;
  friend class ApiTests;
  friend class ApiTests2;
  friend class SystemDevices;

  ComPtr<ID3D12Device>           mDevice;
  ComPtr<ID3D12CommandAllocator> mCommandListAllocator;
  ComPtr<ID3D12Heap>             m_heap;
  ComPtr<ID3D12RootSignature>    m_gRootSig;

  GpuDevice(ComPtr<ID3D12Device> device);
public:
  SwapChain createSwapChain(Window& wnd, GpuCommandQueue& queue);
  GpuFence createFence();
  GpuCommandQueue createQueue();
  GfxCommandList createUniversalCommandList();
  void doExperiment();
  EvolRes CommittedResTest();
  ID3D12Heap* heapCreationTest();
  void RunApiTestCoverage(std::string gpuDescription);

  // Pipelines
private:
  std::wstring s2ws(const std::string& s)
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
public:
  ComputePipeline createComputePipeline(ComputePipelineDescriptor desc)
  {
	  ID3DBlob* blobCompute;
	  ID3DBlob* errorMsg;

    auto woot = s2ws(desc.shaderSourcePath);
	  HRESULT hr = D3DCompileFromFile(woot.c_str(), nullptr, nullptr, "CSMain", "cs_5_1", 0, 0, &blobCompute, &errorMsg);
	  //HRESULT hr = D3DCompileFromFile(L"compute_1.hlsl", nullptr, nullptr, "CSMain", "cs_5_1", 0, 0, &blobCompute, &errorMsg);
	  // https://msdn.microsoft.com/en-us/library/dn859356(v=vs.85).aspx
	  if (FAILED(hr))
	  {
		  if (errorMsg)
		  {
			  OutputDebugStringA((char*)errorMsg->GetBufferPointer());
			  errorMsg->Release();
		  }
      abort();
	  }
	  D3D12_SHADER_BYTECODE byte;
	  byte.pShaderBytecode = blobCompute->GetBufferPointer();
	  byte.BytecodeLength = blobCompute->GetBufferSize();
    if (!m_gRootSig.get())
    {
      ComPtr<ID3D12RootSignatureDeserializer> asd;
      hr = D3D12CreateRootSignatureDeserializer(blobCompute->GetBufferPointer(), blobCompute->GetBufferSize(), __uuidof(ID3D12RootSignatureDeserializer), reinterpret_cast<void**>(asd.addr()));
      if (FAILED(hr))
      {
        abort();
      }
      ComPtr<ID3DBlob> blobSig;
      ComPtr<ID3DBlob> errorSig;
      const D3D12_ROOT_SIGNATURE_DESC* woot = asd->GetRootSignatureDesc();

      hr = D3D12SerializeRootSignature(woot, D3D_ROOT_SIGNATURE_VERSION_1, blobSig.addr(), errorSig.addr());
      if (FAILED(hr))
      {
        abort();
      }

      hr = mDevice->CreateRootSignature(
        1, blobSig->GetBufferPointer(), blobSig->GetBufferSize(),
        __uuidof(ID3D12RootSignature), reinterpret_cast<void**>(m_gRootSig.addr()));
      if (FAILED(hr))
      {
        abort();
      }
    }
    D3D12_COMPUTE_PIPELINE_STATE_DESC computeDesc;
    ZeroMemory(&computeDesc, sizeof(computeDesc));
    computeDesc.CS = byte;
    computeDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    computeDesc.NodeMask = 0;
    computeDesc.pRootSignature = m_gRootSig.get();

    ComPtr<ID3D12PipelineState> pipeline;
    hr = mDevice->CreateComputePipelineState(&computeDesc, __uuidof(ID3D12PipelineState), reinterpret_cast<void**>(pipeline.get()));
    if (FAILED(hr))
    {
      abort();
    }
    ComputePipeline pipe(pipeline);
    return pipe;
  }

  GfxPipeline     createGfxPipeline();

  // buffers
private:
  template <typename ...Args>
  void createBuffer(D3D12_RESOURCE_DESC& desc, D3D12_HEAP_PROPERTIES& prop, Args&& ... args)
  {
    ZeroMemory(&desc, sizeof(desc));
    ZeroMemory(&prop, sizeof(prop));
    // sensible defaults I guess
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.Width = 1;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    prop.Type = D3D12_HEAP_TYPE_DEFAULT;
    fillData(desc, prop, std::forward<Args>(args)...);

  }
public:
  template <typename ...Args>
  BufferSRV createBufferSRV(Args&& ... args)
  {
    D3D12_RESOURCE_DESC desc;
    D3D12_HEAP_PROPERTIES heapprop;
    createBuffer(desc, heapprop, std::forward<Args>(args)...);
    ComPtr<ID3D12Resource> ptr;
    BufferSRV buf;
    buf.buffer().size = desc.Width;
    desc.Width *= desc.DepthOrArraySize;
    buf.buffer().stride = desc.DepthOrArraySize;
    desc.DepthOrArraySize = 1;
    buf.buffer().state = D3D12_RESOURCE_STATE_GENERIC_READ;
    buf.buffer().type = ResType::Gpu;
    if (heapprop.Type == D3D12_HEAP_TYPE_UPLOAD)
    {
      buf.buffer().state = D3D12_RESOURCE_STATE_GENERIC_READ;
      buf.buffer().type = ResType::Upload;
    }
    else if (heapprop.Type == D3D12_HEAP_TYPE_READBACK)
    {
      buf.buffer().state = D3D12_RESOURCE_STATE_COPY_DEST;
      buf.buffer().type = ResType::Readback;
    }
    HRESULT hr = mDevice->CreateCommittedResource(&heapprop,
      D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &desc,
      buf.buffer().state, nullptr, __uuidof(ID3D12Resource),
      reinterpret_cast<void**>(ptr.addr()));

    if (!FAILED(hr))
    {
      buf.buffer().m_resource = std::move(ptr);
    }
    return std::move(buf);
  }

  template <typename ...Args>
  BufferUAV createBufferUAV(Args&& ... args)
  {
    D3D12_RESOURCE_DESC desc;
    D3D12_HEAP_PROPERTIES heapprop;
    createBuffer(desc, heapprop, std::forward<Args>(args)...);
    ComPtr<ID3D12Resource> ptr;
    desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    BufferUAV buf;
    buf.buffer().size = desc.Width;
    desc.Width *= desc.DepthOrArraySize;
    buf.buffer().stride = desc.DepthOrArraySize;
    desc.DepthOrArraySize = 1;
    buf.buffer().state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    buf.buffer().type = ResType::Gpu;
    if (heapprop.Type == D3D12_HEAP_TYPE_UPLOAD)
    {
      buf.buffer().state = D3D12_RESOURCE_STATE_GENERIC_READ;
      buf.buffer().type = ResType::Upload;
    }
    else if (heapprop.Type == D3D12_HEAP_TYPE_READBACK)
    {
      buf.buffer().state = D3D12_RESOURCE_STATE_COPY_DEST;
      buf.buffer().type = ResType::Readback;
    }
    HRESULT hr = mDevice->CreateCommittedResource(&heapprop, 
      D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &desc,
      buf.buffer().state, nullptr, __uuidof(ID3D12Resource),
      reinterpret_cast<void**>(ptr.addr()));
    if (!FAILED(hr))
    {
      buf.buffer().m_resource = std::move(ptr);
    }
    return std::move(buf);
  }

  template <typename ...Args>
  BufferIBV createBufferIBV(Args&& ... args)
  {
    D3D12_RESOURCE_DESC desc;
    D3D12_HEAP_PROPERTIES heapprop;
    createBuffer(desc, heapprop, std::forward<Args>(args)...);
    ComPtr<ID3D12Resource> ptr;
    BufferIBV buf;
    buf.buffer().size = desc.Width;
    desc.Width *= desc.DepthOrArraySize;
    buf.buffer().stride = desc.DepthOrArraySize;
    desc.DepthOrArraySize = 1;
    buf.buffer().state = D3D12_RESOURCE_STATE_INDEX_BUFFER;
    buf.buffer().type = ResType::Gpu;
    if (heapprop.Type == D3D12_HEAP_TYPE_UPLOAD)
    {
      buf.buffer().state = D3D12_RESOURCE_STATE_GENERIC_READ;
      buf.buffer().type = ResType::Upload;
    }
    else if (heapprop.Type == D3D12_HEAP_TYPE_READBACK)
    {
      buf.buffer().state = D3D12_RESOURCE_STATE_COPY_DEST;
      buf.buffer().type = ResType::Readback;
    }
    HRESULT hr = mDevice->CreateCommittedResource(&heapprop,
      D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &desc,
      buf.buffer().state, nullptr, __uuidof(ID3D12Resource),
      reinterpret_cast<void**>(ptr.addr()));

    if (!FAILED(hr))
    {
      buf.buffer().m_resource = std::move(ptr);
    }
    return std::move(buf);
  }

  // TextureSRV, TextureUAV, TextureDSV, TextureRTV
  Texture createTexture();
  TextureSRV createTextureSRV();
  TextureUAV createTextureUAV();
  TextureRTV createTextureRTV();
  TextureDSV createTextureDSV();

  bool isValid()
  {
    return mDevice.get() != nullptr;
  }
};

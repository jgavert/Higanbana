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
#include "GpuResourceViewHeaper.hpp"
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
  
  ResourceViewManager            m_descHeap;
  ResourceViewManager            m_descRTVHeap;
  ResourceViewManager            m_descDSVHeap;

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
  ComputePipeline createComputePipeline(ComputePipelineDescriptor desc)
  {
    ComPtr<ID3D12RootSignature>    m_gRootSig;
	  ComPtr<ID3DBlob> blobCompute;
    auto woot = stringutils::s2ws(desc.shaderSourcePath);
    shaderUtils::getShaderInfo(mDevice, ShaderType::Compute, woot, m_gRootSig, blobCompute);
    D3D12_SHADER_BYTECODE byte;
    byte.pShaderBytecode = blobCompute->GetBufferPointer();
    byte.BytecodeLength = blobCompute->GetBufferSize();
    D3D12_COMPUTE_PIPELINE_STATE_DESC computeDesc;
    ZeroMemory(&computeDesc, sizeof(computeDesc));
    computeDesc.CS = byte;
    computeDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    computeDesc.NodeMask = 0;
    computeDesc.pRootSignature = m_gRootSig.get();

    ComPtr<ID3D12PipelineState> pipeline;
    HRESULT hr = mDevice->CreateComputePipelineState(&computeDesc, __uuidof(ID3D12PipelineState), reinterpret_cast<void**>(pipeline.addr()));
    if (FAILED(hr))
    {
      abort();
    }

    // reflect the root signature
    ComPtr<ID3D12RootSignatureDeserializer> asd;
    hr = D3D12CreateRootSignatureDeserializer(blobCompute->GetBufferPointer(), blobCompute->GetBufferSize(), __uuidof(ID3D12RootSignatureDeserializer), reinterpret_cast<void**>(asd.addr()));
    if (FAILED(hr))
    {
      abort();
    }
    const D3D12_ROOT_SIGNATURE_DESC* woot2 = asd->GetRootSignatureDesc();

    size_t cbv = 0, srv = 0, uav = 0;
    auto bindingInput = shaderUtils::getRootDescriptorReflection(woot2, cbv, srv, uav);
    ComputeBinding sourceBinding(bindingInput, cbv, srv, uav);
    return ComputePipeline(pipeline, m_gRootSig, m_descHeap.m_descHeap, sourceBinding);
  }

  GraphicsPipeline createGraphicsPipeline(GraphicsPipelineDescriptor desc)
  {
    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsDesc = desc.getDesc();
    ComPtr<ID3D12RootSignature>    m_gRootSig;
    ComPtr<ID3DBlob> blobVertex;
    ComPtr<ID3DBlob> blobPixel;
    ComPtr<ID3DBlob> blobGeometry;
    ComPtr<ID3DBlob> blobHull;
    ComPtr<ID3DBlob> blobDomain;

    auto lam = [&](ComPtr<ID3DBlob>& blob, std::string shaderPath, ShaderType type)
    {
      if (!shaderPath.empty())
      {
        auto woot = stringutils::s2ws(shaderPath);
        shaderUtils::getShaderInfo(mDevice, type, woot, m_gRootSig, blob);
      }
    };
    lam(blobVertex, desc.vertexShaderPath, ShaderType::Vertex);
    lam(blobPixel, desc.pixelShaderPath, ShaderType::Pixel);
    lam(blobGeometry, desc.geometryShaderPath, ShaderType::Geometry);
    lam(blobHull, desc.hullShaderPath, ShaderType::Hull);
    lam(blobDomain, desc.domainShaderPath, ShaderType::Domain);
    
    auto modi = [](D3D12_SHADER_BYTECODE& byte, ComPtr<ID3DBlob> blob)
    {
      if (blob.get() != nullptr)
      {
        byte.BytecodeLength = blob->GetBufferSize();
        byte.pShaderBytecode = blob->GetBufferPointer();
      }
    };

    modi(graphicsDesc.VS, blobVertex);
    modi(graphicsDesc.PS, blobPixel);
    modi(graphicsDesc.GS, blobGeometry);
    modi(graphicsDesc.HS, blobHull);
    modi(graphicsDesc.DS, blobDomain);
    graphicsDesc.pRootSignature = m_gRootSig.get();
    ComPtr<ID3D12PipelineState> pipeline;
    HRESULT hr = mDevice->CreateGraphicsPipelineState(&graphicsDesc, __uuidof(ID3D12PipelineState), reinterpret_cast<void**>(pipeline.addr()));
    if (FAILED(hr))
    {
      abort();
    }

    // reflect the root signature
    ComPtr<ID3D12RootSignatureDeserializer> asd;
    hr = D3D12CreateRootSignatureDeserializer(blobVertex->GetBufferPointer(), blobVertex->GetBufferSize(), __uuidof(ID3D12RootSignatureDeserializer), reinterpret_cast<void**>(asd.addr()));
    if (FAILED(hr))
    {
      abort();
    }
    const D3D12_ROOT_SIGNATURE_DESC* woot2 = asd->GetRootSignatureDesc();

    size_t cbv = 0, srv = 0, uav = 0;
    auto bindingInput = shaderUtils::getRootDescriptorReflection(woot2, cbv, srv, uav);
    GraphicsBinding sourceBinding(bindingInput, cbv, srv, uav);

    return GraphicsPipeline(pipeline, m_gRootSig, m_descHeap.m_descHeap, sourceBinding);
  }

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
    buf.buffer().type = ResUsage::Gpu;
    if (heapprop.Type == D3D12_HEAP_TYPE_UPLOAD)
    {
      buf.buffer().state = D3D12_RESOURCE_STATE_GENERIC_READ;
      buf.buffer().type = ResUsage::Upload;
    }
    else if (heapprop.Type == D3D12_HEAP_TYPE_READBACK)
    {
      buf.buffer().state = D3D12_RESOURCE_STATE_COPY_DEST;
      buf.buffer().type = ResUsage::Readback;
    }
    HRESULT hr = mDevice->CreateCommittedResource(&heapprop,
      D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &desc,
      buf.buffer().state, nullptr, __uuidof(ID3D12Resource),
      reinterpret_cast<void**>(ptr.addr()));

    if (!FAILED(hr))
    {
      buf.buffer().m_resource = std::move(ptr);
    }
    D3D12_BUFFER_SRV bufferSRV;
    bufferSRV.FirstElement = 0;
    bufferSRV.NumElements = buf.buffer().size;
    bufferSRV.StructureByteStride = buf.buffer().stride;
    bufferSRV.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    D3D12_SHADER_RESOURCE_VIEW_DESC shaderSRV;
    shaderSRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    shaderSRV.Buffer = bufferSRV;
    shaderSRV.Format = DXGI_FORMAT_UNKNOWN;
    shaderSRV.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;

    BufferView& view = buf.buffer().view;
    view.index = m_descHeap.getNextIndex();
    view.cpuHandle.ptr = m_descHeap.m_descHeap->GetCPUDescriptorHandleForHeapStart().ptr + view.index * m_descHeap.m_handleIncrementSize;

    mDevice->CreateShaderResourceView(buf.buffer().m_resource.get(), &shaderSRV, view.cpuHandle);

    view.gpuHandle.ptr = m_descHeap.m_descHeap->GetGPUDescriptorHandleForHeapStart().ptr + view.index*m_descHeap.m_handleIncrementSize;


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
    buf.buffer().type = ResUsage::Gpu;
    if (heapprop.Type == D3D12_HEAP_TYPE_UPLOAD)
    {
      buf.buffer().state = D3D12_RESOURCE_STATE_GENERIC_READ;
      buf.buffer().type = ResUsage::Upload;
    }
    else if (heapprop.Type == D3D12_HEAP_TYPE_READBACK)
    {
      buf.buffer().state = D3D12_RESOURCE_STATE_COPY_DEST;
      buf.buffer().type = ResUsage::Readback;
    }
    HRESULT hr = mDevice->CreateCommittedResource(&heapprop, 
      D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &desc,
      buf.buffer().state, nullptr, __uuidof(ID3D12Resource),
      reinterpret_cast<void**>(ptr.addr()));
    if (!FAILED(hr))
    {
      buf.buffer().m_resource = std::move(ptr);
    }

    // create view assdfoöuihsdagåöouihgasdöbhujasdgpöbujhkasdgöhujikoad
    D3D12_BUFFER_UAV bufferUAV;
    bufferUAV.FirstElement = 0;
    bufferUAV.NumElements = buf.buffer().size;
    bufferUAV.StructureByteStride = buf.buffer().stride;
    bufferUAV.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
    bufferUAV.CounterOffsetInBytes = 0;

    D3D12_UNORDERED_ACCESS_VIEW_DESC shaderUAV;
    shaderUAV.Buffer = bufferUAV;
    shaderUAV.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    shaderUAV.Format = DXGI_FORMAT_UNKNOWN;

    BufferView& view = buf.buffer().view;
    view.index = m_descHeap.getNextIndex();
    view.cpuHandle.ptr = m_descHeap.m_descHeap->GetCPUDescriptorHandleForHeapStart().ptr + view.index * m_descHeap.m_handleIncrementSize;

    mDevice->CreateUnorderedAccessView(buf.buffer().m_resource.get(), nullptr, &shaderUAV, view.cpuHandle);

    view.gpuHandle.ptr = m_descHeap.m_descHeap->GetGPUDescriptorHandleForHeapStart().ptr + view.index*m_descHeap.m_handleIncrementSize;

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
    buf.buffer().type = ResUsage::Gpu;
    if (heapprop.Type == D3D12_HEAP_TYPE_UPLOAD)
    {
      buf.buffer().state = D3D12_RESOURCE_STATE_GENERIC_READ;
      buf.buffer().type = ResUsage::Upload;
    }
    else if (heapprop.Type == D3D12_HEAP_TYPE_READBACK)
    {
      buf.buffer().state = D3D12_RESOURCE_STATE_COPY_DEST;
      buf.buffer().type = ResUsage::Readback;
    }
    HRESULT hr = mDevice->CreateCommittedResource(&heapprop,
      D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &desc,
      buf.buffer().state, nullptr, __uuidof(ID3D12Resource),
      reinterpret_cast<void**>(ptr.addr()));

    if (!FAILED(hr))
    {
      buf.buffer().m_resource = std::move(ptr);
    }

    // !?!?!?!?! how to bind or use !? wtf
    return std::move(buf);
  }

  private:
    template <typename ...Args>
    void createTexture(D3D12_RESOURCE_DESC& desc, D3D12_HEAP_PROPERTIES& prop, Args&& ... args)
    {
      ZeroMemory(&desc, sizeof(desc));
      ZeroMemory(&prop, sizeof(prop));
      // sensible defaults I guess
      desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
      desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
      desc.Width = 1;
      desc.Height = 1;
      desc.DepthOrArraySize = 1;
      desc.MipLevels = 1;
      desc.SampleDesc.Count = 1;
      desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
      prop.Type = D3D12_HEAP_TYPE_DEFAULT;
      fillData(desc, prop, std::forward<Args>(args)...);

    };
  public:

  template <typename ...Args>
  TextureSRV createTextureSRV(Args&& ... args)
  {
    D3D12_RESOURCE_DESC desc;
    D3D12_HEAP_PROPERTIES heapprop;
    createTexture(desc, heapprop, std::forward<Args>(args)...);

    ComPtr<ID3D12Resource> ptr;
    TextureSRV buf;
    buf.texture().width = desc.Width;
    buf.texture().height = desc.Height;
    desc.Width *= desc.DepthOrArraySize;
    buf.texture().stride = desc.DepthOrArraySize;
    desc.DepthOrArraySize = 1;
    buf.texture().state = D3D12_RESOURCE_STATE_GENERIC_READ;
    buf.texture().type = ResUsage::Gpu;
    if (heapprop.Type == D3D12_HEAP_TYPE_UPLOAD)
    {
      buf.texture().state = D3D12_RESOURCE_STATE_GENERIC_READ;
      buf.texture().type = ResUsage::Upload;
    }
    else if (heapprop.Type == D3D12_HEAP_TYPE_READBACK)
    {
      buf.texture().state = D3D12_RESOURCE_STATE_COPY_DEST;
      buf.texture().type = ResUsage::Readback;
    }
    HRESULT hr = mDevice->CreateCommittedResource(&heapprop,
      D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &desc,
      buf.texture().state, nullptr, __uuidof(ID3D12Resource),
      reinterpret_cast<void**>(ptr.addr()));

    if (!FAILED(hr))
    {
      buf.texture().m_resource = std::move(ptr);
    }
    return buf;
  }

  template <typename ...Args>
  TextureUAV createTextureUAV(Args&& ... args)
  {
    D3D12_RESOURCE_DESC desc;
    D3D12_HEAP_PROPERTIES heapprop;
    createTexture(desc, heapprop, std::forward<Args>(args)...);

    ComPtr<ID3D12Resource> ptr;
    desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    TextureUAV buf;
    buf.texture().width = desc.Width;
    buf.texture().height = desc.Height;
    desc.Width *= desc.DepthOrArraySize;
    buf.texture().stride = desc.DepthOrArraySize;
    desc.DepthOrArraySize = 1;
    buf.texture().state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    buf.texture().type = ResUsage::Gpu;
    if (heapprop.Type == D3D12_HEAP_TYPE_UPLOAD)
    {
      buf.texture().state = D3D12_RESOURCE_STATE_GENERIC_READ;
      buf.texture().type = ResUsage::Upload;
    }
    else if (heapprop.Type == D3D12_HEAP_TYPE_READBACK)
    {
      buf.texture().state = D3D12_RESOURCE_STATE_COPY_DEST;
      buf.texture().type = ResUsage::Readback;
    }
    HRESULT hr = mDevice->CreateCommittedResource(&heapprop,
      D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &desc,
      buf.texture().state, nullptr, __uuidof(ID3D12Resource),
      reinterpret_cast<void**>(ptr.addr()));
    if (!FAILED(hr))
    {
      buf.texture().m_resource = std::move(ptr);
    }

    return buf;
  }
  template <typename ...Args>
  TextureRTV createTextureRTV(Args&& ... args)
  {
    D3D12_RESOURCE_DESC desc;
    D3D12_HEAP_PROPERTIES heapprop;
    createTexture(desc, heapprop, std::forward<Args>(args)...);

    ComPtr<ID3D12Resource> ptr;
    desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    TextureRTV buf;
    buf.texture().width = desc.Width;
    buf.texture().height = desc.Height;
    desc.Width *= desc.DepthOrArraySize;
    buf.texture().stride = desc.DepthOrArraySize;
    desc.DepthOrArraySize = 1;
    buf.texture().state = D3D12_RESOURCE_STATE_RENDER_TARGET;
    buf.texture().type = ResUsage::Gpu;
    // !? wtf
    if (heapprop.Type == D3D12_HEAP_TYPE_UPLOAD)
    {
      buf.texture().state = D3D12_RESOURCE_STATE_GENERIC_READ;
      buf.texture().type = ResUsage::Upload;
    }
    else if (heapprop.Type == D3D12_HEAP_TYPE_READBACK)
    {
      buf.texture().state = D3D12_RESOURCE_STATE_COPY_DEST;
      buf.texture().type = ResUsage::Readback;
    }
    HRESULT hr = mDevice->CreateCommittedResource(&heapprop,
      D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &desc,
      buf.texture().state, nullptr, __uuidof(ID3D12Resource),
      reinterpret_cast<void**>(ptr.addr()));
    if (!FAILED(hr))
    {
      buf.texture().m_resource = std::move(ptr);
    }

    return buf;
  }
  template <typename ...Args>
  TextureDSV createTextureDSV(Args&& ... args)
  {
    D3D12_RESOURCE_DESC desc;
    D3D12_HEAP_PROPERTIES heapprop;
    createTexture(desc, heapprop, std::forward<Args>(args)...);


    ComPtr<ID3D12Resource> ptr;
    desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    TextureDSV buf;
    buf.texture().width = desc.Width;
    buf.texture().height = desc.Height;
    desc.Width *= desc.DepthOrArraySize;
    buf.texture().stride = desc.DepthOrArraySize;
    desc.DepthOrArraySize = 1;
    buf.texture().state = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    buf.texture().type = ResUsage::Gpu;
    if (heapprop.Type == D3D12_HEAP_TYPE_UPLOAD)
    {
      buf.texture().state = D3D12_RESOURCE_STATE_GENERIC_READ;
      buf.texture().type = ResUsage::Upload;
    }
    else if (heapprop.Type == D3D12_HEAP_TYPE_READBACK)
    {
      buf.texture().state = D3D12_RESOURCE_STATE_COPY_DEST;
      buf.texture().type = ResUsage::Readback;
    }
    HRESULT hr = mDevice->CreateCommittedResource(&heapprop,
      D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &desc,
      buf.texture().state, nullptr, __uuidof(ID3D12Resource),
      reinterpret_cast<void**>(ptr.addr()));
    if (!FAILED(hr))
    {
      buf.texture().m_resource = std::move(ptr);
    }

    return buf;
  }

  bool isValid()
  {
    return mDevice.get() != nullptr;
  }

  void resetCmdList(GfxCommandList& list)
  {
    mCommandListAllocator->Reset();
    list.m_CommandList->Reset(mCommandListAllocator.get(), NULL);
  }

  SwapChain createSwapChain(GpuCommandQueue queue, Window& window, unsigned int bufferCount = 8, FormatType type = FormatType::R8G8B8A8_UNORM)
  {
    assert(bufferCount < 9);
    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
    swapChainDesc.BufferCount = bufferCount;
    swapChainDesc.BufferDesc.Format = FormatToDXGIFormat[type];
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = window.getNative();
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    ComPtr<IDXGIFactory4> dxgiFactory = nullptr;
    HRESULT hr = CreateDXGIFactory2(0, __uuidof(IDXGIFactory4), (void**)dxgiFactory.addr());
    assert(!FAILED(hr));

    ComPtr<IDXGISwapChain3> mSwapChain = nullptr;
    hr = dxgiFactory->CreateSwapChain(queue.get().get(), &swapChainDesc, (IDXGISwapChain**)mSwapChain.addr());
    assert(!FAILED(hr));

    // need the rtv's out

    D3D12_CPU_DESCRIPTOR_HANDLE mRenderTargetView[8];
    UINT HandleIncrementSize = m_descRTVHeap.m_handleIncrementSize;
    mRenderTargetView[0] = m_descRTVHeap.m_descHeap->GetCPUDescriptorHandleForHeapStart();
    for (int i = 1;i < bufferCount;++i)
    {
      //create cpu descriptor handle for backbuffer 0
      mRenderTargetView[i].ptr = mRenderTargetView[0].ptr + i * HandleIncrementSize;
    }
    //create cpu descriptor handle for backbuffer 1, offset by D3D12_RTV_DESCRIPTOR_HEAP from backbuffer 0's descriptor

    TextureRTV buf;
    buf.texture().width = swapChainDesc.BufferDesc.Width;
    buf.texture().height = swapChainDesc.BufferDesc.Height;
    buf.texture().stride = 1;
    buf.texture().state = D3D12_RESOURCE_STATE_RENDER_TARGET;
    buf.texture().type = ResUsage::Gpu;

    buf.texture().view.cpuHandle = mRenderTargetView[0];
    buf.texture().view.index = 0;

    std::vector<TextureRTV> lol;
    ID3D12Resource* mRenderTarget;
    //A buffer is required to render to.This example shows how to create that buffer by using the swap chain and device.
    //This example shows calling ID3D12Device::CreateRenderTargetView.

    for (int i = 0;i < bufferCount;++i)
    {
      hr = mSwapChain->GetBuffer(i, __uuidof(ID3D12Resource), (LPVOID*)&mRenderTarget);
      //mRenderTarget->SetName(L"mRenderTarget0");  //set debug name 
      mDevice->CreateRenderTargetView(mRenderTarget, nullptr, mRenderTargetView[i]);
      buf.m_scRTV = mRenderTarget;
      buf.texture().view.cpuHandle = mRenderTargetView[i];
      buf.texture().view.index = i;
      lol.push_back(buf);
    }


    return SwapChain(mSwapChain, lol);
  }

};

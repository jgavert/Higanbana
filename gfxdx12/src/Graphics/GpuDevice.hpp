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
#include "core/src/Platform/Window.hpp"
#include "helpers.hpp"
#include "Descriptors/Descriptors.hpp"
#include "DescriptorHeapManager.hpp"

#include <d3d12.h>
#include <D3Dcompiler.h>
#include <string>

class GpuDevice
{
private:
  friend class test;
  friend class ApiTests;
  friend class ApiTests2;
  friend class System_devices;

  FazCPtr<ID3D12Device>         m_device;
  bool                          m_debugLayer;
  DescriptorHeapManager         m_descHeaps;
  std::vector<ShaderInterface>  m_shaderInterfaceCache;
  TextureNew				m_null;

public:
  GpuDevice(FazCPtr<ID3D12Device> device, bool debugLayer);
  ~GpuDevice();
  SwapChain createSwapChain(Window& wnd, GpuCommandQueue& queue);
  GpuFence createFence();
  GpuCommandQueue createQueue();
  GfxCommandList createUniversalCommandList();
  ComputePipeline createComputePipeline(ComputePipelineDescriptor desc);
  GraphicsPipeline createGraphicsPipeline(GraphicsPipelineDescriptor desc);

  // raw resources
  BufferNew createBuffer(ResourceDescriptor desc);
  TextureNew createTexture(ResourceDescriptor desc);
private:
  D3D12_SHADER_RESOURCE_VIEW_DESC createSrvDesc(ResourceDescriptor& desc, ShaderViewDescriptor& viewDesc);
  D3D12_UNORDERED_ACCESS_VIEW_DESC createUavDesc(ResourceDescriptor& desc, ShaderViewDescriptor& viewDesc);
  D3D12_RENDER_TARGET_VIEW_DESC createRtvDesc(ResourceDescriptor& desc, ShaderViewDescriptor& viewDesc);
  D3D12_DEPTH_STENCIL_VIEW_DESC createDsvDesc(ResourceDescriptor& desc, ShaderViewDescriptor& viewDesc);
public:
  // shader views
  TextureNewSRV createTextureSRV(TextureNew targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  TextureNewUAV createTextureUAV(TextureNew targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  TextureNewRTV createTextureRTV(TextureNew targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  TextureNewDSV createTextureDSV(TextureNew targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());

  BufferNewSRV createBufferSRV(BufferNew targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  BufferNewUAV createBufferUAV(BufferNew targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  BufferNewCBV createBufferCBV(BufferNew targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  BufferNewIBV createBufferIBV(BufferNew targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());

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

    FazCPtr<ID3D12Resource> ptr;
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

    HRESULT hr = m_device->CreateCommittedResource(&heapprop,
      D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &desc,
      buf.texture().state, nullptr, __uuidof(ID3D12Resource),
      reinterpret_cast<void**>(ptr.addr()));

    if (!FAILED(hr))
    {
      buf.texture().m_resource = std::move(ptr);
    }

    auto& m_descSRVHeap = m_descHeaps.getSRV();

    UINT HandleIncrementSize = static_cast<unsigned int>(m_descSRVHeap.m_handleIncrementSize);
    auto lol = m_descSRVHeap.m_descHeap->GetCPUDescriptorHandleForHeapStart();
    auto index = m_descSRVHeap.getSRVIndex();
    buf.texture().view.cpuHandle.ptr = lol.ptr + index * HandleIncrementSize;
    buf.texture().view.index = index;
    buf.texture().shader_heap_index = index - m_descSRVHeap.getSRVStartIndex();
    m_device->CreateShaderResourceView(buf.texture().m_resource.get(), nullptr, buf.texture().view.getCpuHandle());

    return buf;
  }

  template <typename ...Args>
  TextureUAV createTextureUAV(Args&& ... args)
  {
    D3D12_RESOURCE_DESC desc;
    D3D12_HEAP_PROPERTIES heapprop;
    createTexture(desc, heapprop, std::forward<Args>(args)...);

    FazCPtr<ID3D12Resource> ptr;
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

    HRESULT hr = m_device->CreateCommittedResource(&heapprop,
      D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &desc,
      buf.texture().state, nullptr, __uuidof(ID3D12Resource),
      reinterpret_cast<void**>(ptr.addr()));
    if (!FAILED(hr))
    {
      buf.texture().m_resource = std::move(ptr);
    }

    auto& m_descUAVHeap = m_descHeaps.getUAV();

    UINT HandleIncrementSize = static_cast<unsigned int>(m_descUAVHeap.m_handleIncrementSize);
    auto lol = m_descUAVHeap.m_descHeap->GetCPUDescriptorHandleForHeapStart();
    auto index = m_descUAVHeap.getUAVIndex();
    buf.texture().view.cpuHandle.ptr = lol.ptr + index * HandleIncrementSize;
    buf.texture().view.index = index;
    buf.texture().shader_heap_index = index - m_descUAVHeap.getUAVStartIndex(); // FIXME 
    D3D12_CPU_DESCRIPTOR_HANDLE handle;
    handle.ptr = lol.ptr + index * HandleIncrementSize;

    m_device->CreateUnorderedAccessView(buf.texture().m_resource.get(), nullptr,nullptr, handle);

    return buf;
  }

  template <typename ...Args>
  TextureRTV createTextureRTV(Args&& ... args)
  {
    D3D12_RESOURCE_DESC desc;
    D3D12_HEAP_PROPERTIES heapprop;
    createTexture(desc, heapprop, std::forward<Args>(args)...);

    FazCPtr<ID3D12Resource> ptr;
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

    D3D12_CLEAR_VALUE clearVal;
    clearVal.Color[0] = 0.f;
    clearVal.Color[1] = 0.f;
    clearVal.Color[2] = 0.f;
    clearVal.Color[3] = 1.f;
    clearVal.Format = desc.Format;

    HRESULT hr = m_device->CreateCommittedResource(&heapprop,
      D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &desc,
      buf.texture().state, &clearVal, __uuidof(ID3D12Resource),
      reinterpret_cast<void**>(ptr.addr()));
    if (!FAILED(hr))
    {
      buf.texture().m_resource = std::move(ptr);
    }


    auto& m_descRTVHeap = m_descHeaps.getRTV();

    UINT HandleIncrementSize = static_cast<unsigned int>(m_descRTVHeap.m_handleIncrementSize);
    auto lol = m_descRTVHeap.m_descHeap->GetCPUDescriptorHandleForHeapStart();
    auto index = m_descRTVHeap.getNextIndex();
    buf.texture().view.cpuHandle.ptr = lol.ptr + index * HandleIncrementSize;
    buf.texture().view.index = index;
    buf.texture().shader_heap_index = static_cast<unsigned>(index); // FIXME 
    m_device->CreateRenderTargetView(buf.texture().m_resource.get(), nullptr, buf.texture().view.getCpuHandle());

    return buf;
  }
  template <typename ...Args>
  TextureDSV createTextureDSV(Args&& ... args)
  {
    D3D12_RESOURCE_DESC desc;
    D3D12_HEAP_PROPERTIES heapprop;
    createTexture(desc, heapprop, std::forward<Args>(args)...);


    FazCPtr<ID3D12Resource> ptr;
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

    D3D12_CLEAR_VALUE clearVal;
    clearVal.DepthStencil.Depth = 1.f;
    clearVal.DepthStencil.Stencil = 0xff;
    clearVal.Format = desc.Format;

    HRESULT hr = m_device->CreateCommittedResource(&heapprop,
      D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &desc,
      buf.texture().state, &clearVal, __uuidof(ID3D12Resource),
      reinterpret_cast<void**>(ptr.addr()));
    if (!FAILED(hr))
    {
      buf.texture().m_resource = std::move(ptr);
    }

    auto& m_descDSVHeap = m_descHeaps.getDSV();
    UINT HandleIncrementSize = static_cast<unsigned int>(m_descDSVHeap.m_handleIncrementSize);
    auto lol = m_descDSVHeap.m_descHeap->GetCPUDescriptorHandleForHeapStart();
    auto index = m_descDSVHeap.getNextIndex();
    buf.texture().view.cpuHandle.ptr = lol.ptr + index * HandleIncrementSize;
    buf.texture().view.index = index;
    buf.texture().shader_heap_index = static_cast<unsigned>(index); // FIXME 

    D3D12_DEPTH_STENCIL_VIEW_DESC viewdesc;
    viewdesc.Format = desc.Format;
    viewdesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    viewdesc.Texture2D.MipSlice = 0;
    viewdesc.Flags = D3D12_DSV_FLAG_NONE;

    m_device->CreateDepthStencilView(buf.texture().m_resource.get(), &viewdesc, buf.texture().view.getCpuHandle());

    return buf;
  }

  bool isValid()
  {
    return m_device.get() != nullptr;
  }

  // If you want SRGB, https://msdn.microsoft.com/en-us/library/windows/desktop/bb173064.aspx
  // basically create pipeline and pretend that the rtv is SRGB. It will get handled properly.
  SwapChain createSwapChain(GpuCommandQueue queue, Window& window, unsigned int bufferCount = 2, FormatType type = FormatType::R8G8B8A8_UNORM)
  {
    assert(bufferCount < 9);
    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
    swapChainDesc.BufferCount = bufferCount;
    swapChainDesc.BufferDesc.Format = FormatToDXGIFormat[FormatType::R8G8B8A8_UNORM];
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = window.getInternalWindow().getNative();
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    FazCPtr<IDXGIFactory4> dxgiFactory = nullptr;
    HRESULT hr = CreateDXGIFactory2(0, __uuidof(IDXGIFactory4), (void**)dxgiFactory.addr());
    assert(!FAILED(hr));

    FazCPtr<IDXGISwapChain3> mSwapChain = nullptr;
    hr = dxgiFactory->CreateSwapChain(queue.get().get(), &swapChainDesc, (IDXGISwapChain**)mSwapChain.addr());
    assert(!FAILED(hr));

    // need the rtv's out

    D3D12_CPU_DESCRIPTOR_HANDLE mRenderTargetView[8];
    auto& m_descRTVHeap = m_descHeaps.getRTV();
    UINT HandleIncrementSize = static_cast<unsigned int>(m_descRTVHeap.m_handleIncrementSize);
    mRenderTargetView[0] = m_descRTVHeap.m_descHeap->GetCPUDescriptorHandleForHeapStart();
    auto index = m_descRTVHeap.getNextIndex();
    mRenderTargetView[0].ptr = mRenderTargetView[0].ptr + index*HandleIncrementSize;
    std::vector<size_t> indexes;
    indexes.push_back(index);
    for (unsigned int i = 1;i < bufferCount;++i)
    {
      //create cpu descriptor handle for backbuffer 0
      index = m_descRTVHeap.getNextIndex();
      indexes.push_back(index);
      mRenderTargetView[i].ptr = mRenderTargetView[0].ptr + index * HandleIncrementSize;
    }
    //create cpu descriptor handle for backbuffer 1, offset by D3D12_RTV_DESCRIPTOR_HEAP from backbuffer 0's descriptor

    TextureRTV buf;
    buf.texture().width = swapChainDesc.BufferDesc.Width;
    buf.texture().height = swapChainDesc.BufferDesc.Height;
    buf.texture().stride = 1;
    buf.texture().state = D3D12_RESOURCE_STATE_PRESENT;
    buf.texture().type = ResUsage::Gpu;

    buf.texture().view.cpuHandle = mRenderTargetView[0];
    buf.texture().view.index = 0;
    buf.texture().shader_heap_index = 0; // FIXME 

    std::vector<TextureRTV> lol;
    ID3D12Resource* mRenderTarget;
    //A buffer is required to render to.This example shows how to create that buffer by using the swap chain and device.
    //This example shows calling ID3D12Device::CreateRenderTargetView.
    D3D12_RENDER_TARGET_VIEW_DESC desc;
    desc.Format = FormatToDXGIFormat[type];
    desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    desc.Texture2D.MipSlice = 0;
    desc.Texture2D.PlaneSlice = 0;
    for (unsigned int i = 0;i < bufferCount;++i)
    {
      hr = mSwapChain->GetBuffer(i, __uuidof(ID3D12Resource), (LPVOID*)&mRenderTarget);
      //mRenderTarget->SetName(L"mRenderTarget0");  //set debug name 
      m_device->CreateRenderTargetView(mRenderTarget, &desc, mRenderTargetView[i]);
      buf.m_scRTV = mRenderTarget;
      buf.texture().view.cpuHandle = mRenderTargetView[i];
      buf.texture().view.index = indexes[i];
      buf.texture().shader_heap_index = static_cast<int>(indexes[i]); // FIXME 
      lol.push_back(buf);
    }


    return SwapChain(mSwapChain, lol);
  }

  DescriptorHeapManager& getDescHeaps()
  {
    return m_descHeaps;
  }

  ResourceViewManager& getGenericDescriptorHeap()
  {
    return m_descHeaps.getGeneric();
  }

};

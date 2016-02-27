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
  Texture				m_null;

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
  Buffer createBuffer(ResourceDescriptor desc);
  Texture createTexture(ResourceDescriptor desc);
private:
  D3D12_SHADER_RESOURCE_VIEW_DESC createSrvDesc(ResourceDescriptor& desc, ShaderViewDescriptor& viewDesc);
  D3D12_UNORDERED_ACCESS_VIEW_DESC createUavDesc(ResourceDescriptor& desc, ShaderViewDescriptor& viewDesc);
  D3D12_RENDER_TARGET_VIEW_DESC createRtvDesc(ResourceDescriptor& desc, ShaderViewDescriptor& viewDesc);
  D3D12_DEPTH_STENCIL_VIEW_DESC createDsvDesc(ResourceDescriptor& desc, ShaderViewDescriptor& viewDesc);
public:
  // shader views
  TextureSRV createTextureSRV(Texture targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  TextureUAV createTextureUAV(Texture targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  TextureRTV createTextureRTV(Texture targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  TextureDSV createTextureDSV(Texture targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());

  BufferSRV createBufferSRV(Buffer targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  BufferUAV createBufferUAV(Buffer targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  BufferCBV createBufferCBV(Buffer targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  BufferIBV createBufferIBV(Buffer targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  bool isValid() const;
  SwapChain createSwapChain(GpuCommandQueue queue, Window& window, unsigned int bufferCount = 2, FormatType type = FormatType::R8G8B8A8_UNORM);
  DescriptorHeapManager& getDescHeaps();
  ResourceViewManager& getGenericDescriptorHeap();

};

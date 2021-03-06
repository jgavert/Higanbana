#pragma once
#include "higanbana/graphics/common/device_group_data.hpp"
#include "higanbana/graphics/common/helpers/shared_state.hpp"
#include "higanbana/graphics/common/resource_descriptor.hpp"
#include "higanbana/graphics/common/buffer.hpp"
#include "higanbana/graphics/common/texture.hpp"
#include "higanbana/graphics/common/semaphore.hpp"
#include "higanbana/graphics/common/swapchain.hpp"
#include "higanbana/graphics/common/commandgraph.hpp"
#include "higanbana/graphics/common/commandlist.hpp"
#include "higanbana/graphics/common/cpuimage.hpp"
#include "higanbana/graphics/common/resources/shader_arguments.hpp"
#include "higanbana/graphics/desc/shader_arguments_layout_descriptor.hpp"
#include "higanbana/graphics/common/shader_arguments_descriptor.hpp"
#include "higanbana/graphics/common/raytracing_descriptors.hpp"
#include "higanbana/graphics/common/prototypes.hpp"
#include "higanbana/graphics/desc/device_stats.hpp"
#include <higanbana/core/datastructures/deque.hpp>

#include <optional>

namespace higanbana
{
  class LBS;
  class GpuGroup : private backend::SharedState<backend::DeviceGroupData>
  {
  public:
    GpuGroup() = default;
    GpuGroup(vector<std::shared_ptr<backend::prototypes::DeviceImpl>> impl, vector<GpuInfo> infos)
    {
      makeState(impl, infos);
    }
    GpuGroup(StatePtr state)
    {
      m_state = std::move(state);
    }

    StatePtr state()
    {
      return m_state;
    }
    
    Buffer createBuffer(ResourceDescriptor descriptor)
    {
      S().validateResourceDescriptor(descriptor);
      auto buf = S().createBuffer(descriptor);
      return buf;
    }

    Texture createTexture(ResourceDescriptor descriptor)
    {
      S().validateResourceDescriptor(descriptor);
      if (descriptor.desc.dimension == FormatDimension::Buffer
      || descriptor.desc.dimension == FormatDimension::Unknown)
      {
        descriptor = descriptor.setDimension(FormatDimension::Texture2D);
      }
      auto tex = S().createTexture(descriptor);
      return tex;
    }

    Texture createTexture(CpuImage& image)
    {
      auto tex = createTexture(image.desc());
      S().uploadInitialTexture(tex, image);
      return tex;
    }

    BufferIBV createBufferIBV(Buffer texture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor())
    {
      return S().createBufferIBV(texture, viewDesc);
    }

    BufferIBV createBufferIBV(ResourceDescriptor descriptor, ShaderViewDescriptor viewDesc = ShaderViewDescriptor())
    {
      return createBufferIBV(createBuffer(descriptor), viewDesc);
    }

    BufferSRV createBufferSRV(Buffer texture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor())
    {
      return S().createBufferSRV(texture, viewDesc);
    }

    BufferSRV createBufferSRV(ResourceDescriptor descriptor, ShaderViewDescriptor viewDesc = ShaderViewDescriptor())
    {
      return createBufferSRV(createBuffer(descriptor), viewDesc);
    }

    BufferUAV createBufferUAV(Buffer texture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor())
    {
      return S().createBufferUAV(texture, viewDesc);
    }

    BufferRTAS createBufferRTAS(std::string name, size_t sizeBytes) {
      auto descriptor = ResourceDescriptor()
        .setName(name)
        .setElementsCount(sizeBytes)
        .setFormat(FormatType::Unknown)
        .setUsage(ResourceUsage::RTAccelerationStructure);
      return S().createAccelerationStructure(createBuffer(descriptor));
    }

    BufferUAV createBufferUAV(ResourceDescriptor descriptor, ShaderViewDescriptor viewDesc = ShaderViewDescriptor())
    {
      return createBufferUAV(createBuffer(descriptor), viewDesc);
    }

    TextureSRV createTextureSRV(Texture texture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor())
    {
      return S().createTextureSRV(texture, viewDesc);
    }

    TextureSRV createTextureSRV(ResourceDescriptor descriptor, ShaderViewDescriptor viewDesc = ShaderViewDescriptor())
    {
      return createTextureSRV(createTexture(descriptor), viewDesc);
    }

    TextureUAV createTextureUAV(Texture texture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor())
    {
      return S().createTextureUAV(texture, viewDesc);
    }

    TextureUAV createTextureUAV(ResourceDescriptor descriptor, ShaderViewDescriptor viewDesc = ShaderViewDescriptor())
    {
      return createTextureUAV(createTexture(descriptor), viewDesc);
    }

    TextureRTV createTextureRTV(Texture texture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor())
    {
      return S().createTextureRTV(texture, viewDesc);
    }

    TextureRTV createTextureRTV(ResourceDescriptor descriptor, ShaderViewDescriptor viewDesc = ShaderViewDescriptor())
    {
      return createTextureRTV(createTexture(descriptor), viewDesc);
    }

    TextureDSV createTextureDSV(Texture texture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor())
    {
      return S().createTextureDSV(texture, viewDesc);
    }

    TextureDSV createTextureDSV(ResourceDescriptor descriptor, ShaderViewDescriptor viewDesc = ShaderViewDescriptor())
    {
      return createTextureDSV(createTexture(descriptor), viewDesc);
    }

    size_t availableDynamicMemory(int gpu = 0) {
      return S().availableDynamicMemory(gpu);
    }

    template <typename Type>
    DynamicBufferView dynamicBuffer(MemView<Type> view, FormatType type)
    {
      return S().dynamicBuffer(reinterpret_memView<uint8_t>(view), type);
    }

    template <typename Type>
    DynamicBufferView dynamicBuffer(MemView<Type> view)
    {
      return S().dynamicBuffer(reinterpret_memView<uint8_t>(view), sizeof(Type));
    }

    template <typename Type>
    DynamicBufferView dynamicImage(MemView<Type> view, size_t rowPitch)
    {
      return S().dynamicImage(reinterpret_memView<uint8_t>(view), static_cast<unsigned>(rowPitch));
    }

    ShaderArgumentsLayout createShaderArgumentsLayout(ShaderArgumentsLayoutDescriptor desc)
    {
      return S().createShaderArgumentsLayout(desc);
    }

    ShaderArguments createShaderArguments(ShaderArgumentsDescriptor& binding)
    {
      return S().createShaderArguments(binding);
    }

    Renderpass createRenderpass()
    {
      return S().createRenderpass();
    }

    ComputePipeline createComputePipeline(ComputePipelineDescriptor desc)
    {
      return S().createComputePipeline(desc);
    }

    GraphicsPipeline createGraphicsPipeline(GraphicsPipelineDescriptor desc)
    {
      return S().createGraphicsPipeline(desc);
    }

    CommandGraph createGraph()
    {
      return S().startCommandGraph();
    }

    Swapchain createSwapchain(GraphicsSurface& surface, SwapchainDescriptor descriptor = SwapchainDescriptor())
    {
      auto sc = S().createSwapchain(surface, descriptor);
      sc.setParent(this);
      return sc;
    }

    void adjustSwapchain(Swapchain& swapchain, SwapchainDescriptor descriptor = SwapchainDescriptor())
    {
      S().adjustSwapchain(swapchain, descriptor);
    }

    std::optional<std::pair<int, TextureRTV>> acquirePresentableImage(Swapchain& swapchain)
    {
      return S().acquirePresentableImage(swapchain);
    }

    TextureRTV* tryAcquirePresentableImage(Swapchain& swapchain)
    {
        return S().tryAcquirePresentableImage(swapchain);
    }
    void submit(Swapchain& swapchain, CommandGraph graph, ThreadedSubmission threading = ThreadedSubmission::ParallelUnsequenced)
    {
      if (threading == ThreadedSubmission::Sequenced)
        S().submitST(swapchain, graph);
      else
        S().submit(swapchain, graph, threading);
    }

    void submit(CommandGraph graph, ThreadedSubmission threading = ThreadedSubmission::ParallelUnsequenced)
    {
      if (threading == ThreadedSubmission::Sequenced)
        S().submitST(std::optional<Swapchain>(), graph);
      else
        S().submit(std::optional<Swapchain>(), graph, threading);
    }

    // this is async present, just doesn't read like it :D
    void present(Swapchain& swapchain, int backbufferIndex)
    {
      S().present(swapchain, backbufferIndex);
    }

    void garbageCollection()
    {
      S().garbageCollection();
    }

#if JGPU_COROUTINES // start of css::Task includes
    css::Task<void> asyncSubmit(Swapchain& swapchain, CommandGraph graph) {
      return S().asyncSubmit(swapchain, graph);
    }
    css::Task<void> asyncSubmit(CommandGraph graph) {
      return S().asyncSubmit(std::optional<Swapchain>(), graph);
    }
    css::Task<void> asyncPresent(Swapchain& swapchain, int backbufferIndex)
    {
      return S().presentAsync(swapchain, backbufferIndex);
    }
#endif

    void waitGpuIdle()
    {
      S().waitGpuIdle();
    }

    bool alive()
    {
      return valid();
    }

    int deviceCount() const {
      return static_cast<int>(S().m_devices.size());
    }

    vector<GpuInfo> activeDevicesInfo() {
      vector<GpuInfo> ret;
      for (auto&& device : S().m_devices) {
        ret.push_back(device.info);
      }
      return ret;
    }

    deque<SubmitTiming> submitTimingInfo()
    {
      auto infos = S().timeSubmitsFinished;
      S().timeSubmitsFinished.clear();
      return infos;
    }

    std::optional<SubmitTiming> lastSubmitTiming()
    {
      if (S().timeSubmitsFinished.empty())
        return std::optional<SubmitTiming>();
      auto info = S().timeSubmitsFinished.front();
      S().timeSubmitsFinished.pop_front();
      return info;
    }

    uint64_t gpuMemoryUsed()
    {
      uint64_t allMemoryUsed = 0;
      for (auto&& dev : S().m_devices)
      {
        allMemoryUsed += dev.heaps.memoryInUse();
      }
      return allMemoryUsed;
    }

    vector<DeviceStatistics> gpuStatistics()
    {
      vector<DeviceStatistics> allMemoryUsed;
      for (auto&& dev : S().m_devices)
      {
        auto stat = dev.device->statsOfResourcesInUse();
        stat.gpuMemoryAllocated = dev.heaps.memoryInUse();
        stat.gpuTotalMemory = dev.heaps.totalMemory();
        stat.commandlistsOnGpu = dev.m_gfxBuffers.size() + dev.m_computeBuffers.size() + dev.m_dmaBuffers.size();
        allMemoryUsed.push_back(stat);
      }
      return allMemoryUsed;
    }

    // raytracing 

    desc::RaytracingASPreBuildInfo accelerationStructurePrebuildInfo(desc::RaytracingAccelerationStructureInputs desc) {
      return S().accelerationStructurePrebuildInfo(desc);
    }
  };
};
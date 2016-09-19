#pragma once
#include "VulkanQueue.hpp"
#include "VulkanCmdBuffer.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanTexture.hpp"
#include "VulkanHeap.hpp"
#include "VulkanPipeline.hpp"
#include "vkShaders\shader_defines.hpp"
#include "core/src/filesystem/filesystem.hpp"
#include "gfxvk/src/Graphics/ResourceDescriptor.hpp"
#include "gfxvk/src/Graphics/PipelineDescriptor.hpp"
#include "gfxvk/src/Graphics/Heap.hpp"
#include "gfxvk/src/Graphics/vk_specifics/shader/ShaderStorage.hpp"
#include <vulkan/vulkan.hpp>
#include <memory>

class VulkanGpuDevice
{
private:


  vk::AllocationCallbacks m_alloc_info;
  std::shared_ptr<vk::Device>    m_device;
  bool                    m_debugLayer;
  std::vector<vk::QueueFamilyProperties> m_queues;
  bool                    m_singleQueue;
  bool                    m_computeQueues;
  bool                    m_dmaQueues;
  bool                    m_graphicQueues;
  std::shared_ptr<vk::Queue>     m_internalUniversalQueue;
  bool                    m_uma;

  ShaderStorage           m_shaders;

  struct FreeQueues
  {
    int universalIndex;
    int graphicsIndex;
    int computeIndex;
    int dmaIndex;
    std::vector<uint32_t> universal;
    std::vector<uint32_t> graphics;
    std::vector<uint32_t> compute;
    std::vector<uint32_t> dma;
  } m_freeQueueIndexes;

  struct MemoryTypes
  {
    int deviceLocalIndex;
    int hostNormalIndex;
    int hostCachedIndex; // probably not needed
    int deviceHostIndex; // default for uma, when discrete has this... what
  } m_memoryTypes;

  struct PipelineLayout
  {
    size_t pushConstantsSize;
    int srvBufferStartIndex;
    int uavBufferStartIndex;
    int srvTextureStartIndex;
    int uavTextureStartIndex;
  };

public:
  VulkanGpuDevice(
	std::shared_ptr<vk::Device> device,
    FileSystem& fs,
    vk::AllocationCallbacks alloc_info,
    std::vector<vk::QueueFamilyProperties> queues,
    vk::PhysicalDeviceMemoryProperties memProp,
    bool debugLayer);
  bool isValid();
  VulkanQueue createDMAQueue();
  VulkanQueue createComputeQueue();
  VulkanQueue createGraphicsQueue();
  VulkanCmdBuffer createDMACommandBuffer();
  VulkanCmdBuffer createComputeCommandBuffer();
  VulkanCmdBuffer createGraphicsCommandBuffer();
  VulkanPipeline createGraphicsPipeline(GraphicsPipelineDescriptor desc);

  template <typename ShaderType>
  VulkanPipeline createComputePipeline(ComputePipelineDescriptor desc)
  {
    PipelineLayout layout;
    layout.pushConstantsSize = ShaderType::pushConstants;
    layout.srvBufferStartIndex = ShaderType::srvBufferStart;
    layout.uavBufferStartIndex = ShaderType::uavBufferStart;
    layout.srvTextureStartIndex = ShaderType::srvTextureStart;
    layout.uavTextureStartIndex = ShaderType::uavTextureStart;
    return createComputePipeline(layout, desc);
  }

  VulkanPipeline createComputePipeline(PipelineLayout layout, ComputePipelineDescriptor desc);
  VulkanMemoryHeap createMemoryHeap(HeapDescriptor desc);
  VulkanBuffer createBuffer(ResourceHeap& heap, ResourceDescriptor desc);
  VulkanTexture createTexture(ResourceHeap& heap, ResourceDescriptor desc);
  // shader views
  VulkanBufferShaderView createBufferView(VulkanBuffer targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  VulkanTextureShaderView createTextureView(VulkanTexture targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());

};

using GpuDeviceImpl = VulkanGpuDevice;

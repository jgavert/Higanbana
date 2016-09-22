#pragma once
#include "VulkanQueue.hpp"
#include "VulkanCmdBuffer.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanTexture.hpp"
#include "VulkanHeap.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanFence.hpp"
#include "vkShaders/shader_defines.hpp"
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
  };

  std::shared_ptr<FreeQueues> m_freeQueueIndexes;

  struct MemoryTypes
  {
    int deviceLocalIndex = -1;
    int hostNormalIndex = -1;
    int hostCachedIndex = -1; // probably not needed
    int deviceHostIndex = -1; // default for uma, when discrete has this... what
  } m_memoryTypes;

  struct ShaderInputLayout 
  {
    size_t pushConstantsSize;
    int srvBufferCount;
    int uavBufferCount;
    int srvTextureCount;
    int uavTextureCount;
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
	ShaderInputLayout layout;
    layout.pushConstantsSize = ShaderType::pushConstantsSize;
    layout.srvBufferCount = ShaderType::s_srvBufferCount;
    layout.uavBufferCount = ShaderType::s_uavBufferCount;
    layout.srvTextureCount = ShaderType::s_srvTextureCount;
    layout.uavTextureCount = ShaderType::s_uavTextureCount;
    return createComputePipeline(layout, desc);
  }

  VulkanPipeline createComputePipeline(ShaderInputLayout layout, ComputePipelineDescriptor desc);
  VulkanMemoryHeap createMemoryHeap(HeapDescriptor desc);
  VulkanBuffer createBuffer(ResourceHeap& heap, ResourceDescriptor desc);
  VulkanTexture createTexture(ResourceHeap& heap, ResourceDescriptor desc);
  // shader views
  VulkanBufferShaderView createBufferView(VulkanBuffer targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  VulkanTextureShaderView createTextureView(VulkanTexture targetTexture, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());

  // synchro creates
  VulkanFence createFence();
  void waitFence(VulkanFence& fence);
  bool checkFence(VulkanFence& fence);
  void waitIdle();

  // resets
  void resetCmdBuffer(VulkanCmdBuffer& buffer);
  void resetFence(VulkanFence& fence);
};

using GpuDeviceImpl = VulkanGpuDevice;

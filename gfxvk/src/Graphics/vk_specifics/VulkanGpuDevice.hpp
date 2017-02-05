#pragma once
#include "VulkanSurface.hpp"
#include "VulkanSwapchain.hpp"
#include "VulkanQueue.hpp"
#include "VulkanCmdBuffer.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanTexture.hpp"
#include "VulkanHeap.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanFence.hpp"
#include "VulkanDescriptorPool.hpp"
#include "VulkanDescriptorSet.hpp"
#include "VulkanSemaphore.hpp"
#include "gfxvk/src/Graphics/SwapChain.hpp"
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
  friend class VulkanCmdBuffer;

  vk::AllocationCallbacks m_alloc_info;
  std::shared_ptr<vk::Device>    m_device;
  vk::PhysicalDevice		m_physDevice;
  bool                    m_debugLayer;
  std::vector<vk::QueueFamilyProperties> m_queues;
  bool                    m_singleQueue;
  bool                    m_computeQueues;
  bool                    m_dmaQueues;
  bool                    m_graphicQueues;
  std::shared_ptr<vk::Queue>     m_internalUniversalQueue;
  bool                    m_uma;

  ShaderStorage           m_shaders;

  int64_t                 m_resourceID = 1;

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
    int deviceLocalIndex = -1; // device local
    int hostNormalIndex = -1; // upload
    int hostCachedIndex = -1; // readback 
    int deviceHostIndex = -1; // default for uma. If this is also existing, you can copy things to gpu directly instead of upload.
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
	  vk::PhysicalDevice physDev,
    FileSystem& fs,
    vk::AllocationCallbacks alloc_info,
    std::vector<vk::QueueFamilyProperties> queues,
    vk::PhysicalDeviceMemoryProperties memProp,
    bool debugLayer);
  bool isValid();

  // create
  std::vector<ResourceDescriptor> querySwapChainInfo(VulkanSurface& surface);
  std::vector<PresentMode> queryPresentModes(VulkanSurface& surface);
  VulkanSwapchain createSwapchain(VulkanSurface& surface, VulkanQueue& queue, FormatType format, PresentMode mode);
  void reCreateSwapchain(VulkanSwapchain& sc, VulkanSurface& surface, VulkanQueue& queue, ResourceDescriptor& descriptor, PresentMode mode);
  std::vector<VulkanTexture> getSwapchainTextures(VulkanSwapchain& sc);

  int acquireNextImage(VulkanSwapchain& sc, VulkanSemaphore& image);

  VulkanSemaphore createSemaphore();

  VulkanQueue createDMAQueue();
  VulkanQueue createComputeQueue();
  VulkanQueue createGraphicsQueue();
  VulkanCmdBuffer createDMACommandBuffer();
  VulkanCmdBuffer createComputeCommandBuffer();
  VulkanCmdBuffer createGraphicsCommandBuffer();
  VulkanDescriptorPool createDescriptorPool();
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
  VulkanBufferShaderView createBufferView(VulkanBuffer& targetTexture, ResourceDescriptor& desc, ResourceShaderType shaderType, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());
  VulkanTextureShaderView createTextureView(VulkanTexture& targetTexture, ResourceDescriptor& desc, ResourceShaderType shaderType, ShaderViewDescriptor viewDesc = ShaderViewDescriptor());

  // synchro creates
  VulkanFence createFence();
  void waitFence(VulkanFence& fence);
  bool checkFence(VulkanFence& fence);
  void waitIdle();

  // resets
  void resetCmdBuffer(VulkanCmdBuffer& buffer);
  void resetFence(VulkanFence& fence);
  void reset(VulkanDescriptorPool& pool);

  // destroys
  void destroy(VulkanDescriptorPool& pool);

  vk::Device& native()
  {
    return *m_device;
  }
};

using GpuDeviceImpl = VulkanGpuDevice;

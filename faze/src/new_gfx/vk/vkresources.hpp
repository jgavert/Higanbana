#pragma once
#include "faze/src/new_gfx/common/prototypes.hpp"
#include "faze/src/new_gfx/common/resources.hpp"

#include "faze/src/new_gfx/vk/util/AllocStuff.hpp" // refactor
#include "faze/src/new_gfx/vk/util/ShaderStorage.hpp"

#include "faze/src/new_gfx/definitions.hpp"

#include <vulkan/vulkan.hpp>

namespace faze
{
  namespace backend
  {
    class VulkanTexture : public prototypes::TextureImpl
    {
    private:
      vk::Image resource;

    public:
      VulkanTexture()
      {}
      VulkanTexture(vk::Image resource)
        : resource(resource)
      {}
      vk::Image native()
      {
        return resource;
      }
    };

    class VulkanBuffer : public prototypes::BufferImpl
    {
    private:
      vk::Buffer resource;

    public:
      VulkanBuffer()
      {}
      VulkanBuffer(vk::Buffer resource)
        : resource(resource)
      {}
      vk::Buffer native()
      {
        return resource;
      }
    };

    class VulkanHeap : public prototypes::HeapImpl
    {
    private:
      vk::DeviceMemory m_resource;

    public:
      VulkanHeap(vk::DeviceMemory memory)
        : m_resource(memory)
      {}

      vk::DeviceMemory native()
      {
        return m_resource;
      }
    };

    class VulkanDevice : public prototypes::DeviceImpl
    {
    private:
      vk::Device                  m_device;
      vk::PhysicalDevice		      m_physDevice;
      bool                        m_debugLayer;
      std::vector<vk::QueueFamilyProperties> m_queues;
      bool                        m_singleQueue;
      bool                        m_computeQueues;
      bool                        m_dmaQueues;
      bool                        m_graphicQueues;
      vk::Queue                   m_internalUniversalQueue;
      GpuInfo                     m_info;
      ShaderStorage               m_shaders;
      int64_t                     m_resourceID = 1;

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
      /*
      struct MemoryTypes
      {
        int deviceLocalIndex = -1;
        int hostNormalIndex = -1;
        int hostCachedIndex = -1; 
        int deviceHostIndex = -1;
      } m_memoryTypes;
      */
    public:
      VulkanDevice(
        vk::Device device,
        vk::PhysicalDevice physDev,
        FileSystem& fs,
        std::vector<vk::QueueFamilyProperties> queues,
        GpuInfo info,
        bool debugLayer);
      ~VulkanDevice();
      
      vk::BufferCreateInfo fillBufferInfo(ResourceDescriptor descriptor);
      vk::ImageCreateInfo fillImageInfo(ResourceDescriptor descriptor);

      // implementation
      void waitGpuIdle() override;
      MemoryRequirements getReqs(ResourceDescriptor desc) override;

      GpuHeap createHeap(HeapDescriptor desc) override;
      void destroyHeap(GpuHeap heap) override;

      std::shared_ptr<prototypes::BufferImpl> createBuffer(HeapAllocation allocation, ResourceDescriptor desc) override;
      void destroyBuffer(std::shared_ptr<prototypes::BufferImpl> buffer) override;
      void createBufferView(ShaderViewDescriptor desc) override;

      std::shared_ptr<prototypes::TextureImpl> createTexture(HeapAllocation allocation, ResourceDescriptor desc);
      void destroyTexture(std::shared_ptr<prototypes::TextureImpl> buffer);
    };

    class VulkanSubsystem : public prototypes::SubsystemImpl
    {
      vector<GpuInfo> infos;

      allocs m_allocs;
      vk::AllocationCallbacks m_alloc_info;
      vk::ApplicationInfo app_info;
      vk::InstanceCreateInfo instance_info;
      vector<vk::ExtensionProperties> m_extensions;
      vector<vk::LayerProperties> m_layers;
      vector<vk::PhysicalDevice> m_devices;
      std::shared_ptr<vk::Instance> m_instance;
      std::shared_ptr<vk::DebugReportCallbackEXT> m_debugcallback;

      // lunargvalidation list order
      std::vector<std::string> layerOrder = {
#if defined(FAZE_GRAPHICS_VALIDATION_LAYER)
        "VK_LAYER_LUNARG_standard_validation",
#endif
        "VK_LAYER_LUNARG_swapchain"
      };

      std::vector<std::string> extOrder = {
        VK_KHR_SURFACE_EXTENSION_NAME
#if defined(PF_WINDOWS)
        , VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#endif
#if defined(FAZE_GRAPHICS_VALIDATION_LAYER)
        , VK_EXT_DEBUG_REPORT_EXTENSION_NAME
#endif
      };

      std::vector<std::string> devExtOrder = {
        "VK_KHR_swapchain"
      };
    public:
      VulkanSubsystem(const char* appName, unsigned appVersion, const char* engineName, unsigned engineVersion);
      std::string gfxApi();
      vector<GpuInfo> availableGpus();
      GpuDevice createGpuDevice(FileSystem& fs, GpuInfo gpu);
    };

  }
}
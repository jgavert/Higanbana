#include "temp.hpp"

#include "core/src/memory/ManagedResource.hpp"

#include <vulkan/vk_cpp.h>
#include <iostream>
#include <memory>

const char* yay::message()
{
  return "Vulkan!";
}

void* pfnAllocation(
  void*                                       /*pUserData*/,
  size_t                                      size, // bytes
  size_t                                      alignment,
  VkSystemAllocationScope                     /*allocationScope*/)
{
  return _aligned_malloc(size, alignment);
};

void* pfnReallocation(
  void*                                       /*pUserData*/,
  void*                                       pOriginal,
  size_t                                      size,
  size_t                                      alignment,
  VkSystemAllocationScope                     /*allocationScope*/)
{
  return _aligned_realloc(pOriginal, size, alignment);
};

void pfnFree(
  void*                                       /*pUserData*/,
  void*                                       pMemory)
{
  _aligned_free(pMemory);
};

void pfnInternalAllocation(
  void*                                       /*pUserData*/,
  size_t                                      /*size*/,
  VkInternalAllocationType                    /*allocationType*/,
  VkSystemAllocationScope                     /*allocationScope*/)
{

};

void pfnInternalFree(
  void*                                       /*pUserData*/,
  size_t                                      /*size*/,
  VkInternalAllocationType                    /*allocationType*/,
  VkSystemAllocationScope                     /*allocationScope*/)
{

};

bool yay::test()
{
  vk::ApplicationInfo app_info = vk::ApplicationInfo()
    .sType(vk::StructureType::eApplicationInfo)
    .pApplicationName("vulkan test")
    .applicationVersion(1)
    .pEngineName("faze")
    .engineVersion(1)
    .apiVersion(VK_API_VERSION);
  
  vk::InstanceCreateInfo inst_info = vk::InstanceCreateInfo()
    .sType(vk::StructureType::eInstanceCreateInfo)
    .pApplicationInfo(&app_info);

  /*
  inst_info.enabledExtensionCount = 0;
  inst_info.ppEnabledExtensionNames = NULL;
  inst_info.enabledLayerCount = 0;
  inst_info.ppEnabledLayerNames = NULL;
  */

  vk::AllocationCallbacks alloc_info(nullptr, pfnAllocation, pfnReallocation, pfnFree, pfnInternalAllocation, pfnInternalFree);

  FazPtr<vk::Instance> instance([=](vk::Instance ist)
  {
    vk::destroyInstance(ist, &alloc_info);
  });

  vk::Result res = vk::createInstance(&inst_info, &alloc_info, instance.get());

  if (res != vk::Result::eVkSuccess)
  {
    // quite hot shit baby yeah!
    std::cout << vk::getString(res) << std::endl;
	  return false;
  }
	return true;
}

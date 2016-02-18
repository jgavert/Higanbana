#include "temp.hpp"

#include "core/src/memory/ManagedResource.hpp"

#include <vulkan/vk_cpp.h>
#include <iostream>
#include <memory>

const char* yay::message()
{
  return "Vulkan!";
}

bool yay::test()
{
  vk::ApplicationInfo app_info = vk::ApplicationInfo()
    .sType(vk::StructureType::eApplicationInfo)
    .pNext(nullptr)
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
  FazPtr<vk::Instance> instance([](vk::Instance ist)
  {
    vk::destroyInstance(ist, nullptr);
  });

  vk::Result res;
  res = vk::createInstance(&inst_info, nullptr, instance.get());
  if (res == vk::Result::eVkErrorIncompatibleDriver) {
    std::cout << "cannot find a compatible Vulkan ICD\n";
	  return false;
  }
  else if (static_cast<int>(res)) {
    std::cout << "unknown error\n";
    return false;
  }
  //vk::destroyInstance(inst, nullptr); // FazPtr handles the destruction.
	return true;
}

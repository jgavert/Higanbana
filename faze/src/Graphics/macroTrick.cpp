#include "macroTrick.hpp"

#include <vulkan/vulkan.hpp>

void _ApiDebug::dbgMsg(const char* msg, int num)
{
  vk::Result result = static_cast<vk::Result>(num);
  auto error = to_string(result);
  F_LOG(msg, error.c_str());
}

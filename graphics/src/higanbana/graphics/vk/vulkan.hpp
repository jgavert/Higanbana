#pragma once

#include <higanbana/core/platform/definitions.hpp>
#ifdef HIGANBANA_PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#define VULKAN_HPP_NO_SMART_HANDLE
#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_DISABLE_IMPLICIT_RESULT_VALUE_CAST
#include <cassert>
#define VULKAN_HPP_ASSERT
#include <vulkan/vulkan.hpp>

#pragma once

#include <higanbana/core/platform/definitions.hpp>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-value"
#pragma clang diagnostic ignored "-Wdefaulted-function-deleted"
// Declarations/definitions
#ifdef HIGANBANA_PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#define VULKAN_HPP_NO_SMART_HANDLE
#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_DISABLE_IMPLICIT_RESULT_VALUE_CAST
#include <cassert>
#define VULKAN_HPP_ASSERT
#include <vulkan/vulkan.hpp>
#pragma clang diagnostic pop

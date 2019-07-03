#pragma once

#include "higanbana/graphics/vk/vkresources.hpp"
#include "higanbana/graphics/vk/vkdevice.hpp"
#include "higanbana/graphics/vk/vksubsystem.hpp"
#if defined(HIGANBANA_PLATFORM_WINDOWS)
#include "higanbana/graphics/dx12/dx12resources.hpp"
#include "higanbana/graphics/dx12/dx12device.hpp"
#include "higanbana/graphics/dx12/dx12subsystem.hpp"
#endif
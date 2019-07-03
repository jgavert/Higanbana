#pragma once

#include "graphics/vk/vkresources.hpp"
#include "graphics/vk/vkdevice.hpp"
#include "graphics/vk/vksubsystem.hpp"
#if defined(FAZE_PLATFORM_WINDOWS)
#include "graphics/dx12/dx12resources.hpp"
#include "graphics/dx12/dx12device.hpp"
#include "graphics/dx12/dx12subsystem.hpp"
#endif
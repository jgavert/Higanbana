#pragma once

// Enables warp driver on dx12, doesn't do anything on vulkan
#define FAZE_GRAPHICS_WARP_DRIVER

// Enables debug validation layers for dx12/vulkan
#define FAZE_GRAPHICS_VALIDATION_LAYER

// Enables GBV validation for DX12, super slow.
#define FAZE_GRAPHICS_GPUBASED_VALIDATION
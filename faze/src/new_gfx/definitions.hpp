#pragma once

// Enables warp driver on dx12, doesn't do anything on vulkan
//#define FAZE_GRAPHICS_WARP_DRIVER

// Enables debug validation layers for dx12/vulkan
//#define FAZE_GRAPHICS_VALIDATION_LAYER

// Enables GBV validation for DX12, super slow.
//#define FAZE_GRAPHICS_GPUBASED_VALIDATION

// Could probably convert above to be global booleans instead, so I could toggle them runtime.
// Though many of them require to create all resources from scratch.

namespace faze
{
    namespace globalconfig 
    {
        namespace graphics
        {

            // barrier booleans, toggle to control. Nothing thread safe here.
            // cannot toggle per window... I guess.
            static bool GraphicsEnableReadStateCombining = false;
        }
    }
}
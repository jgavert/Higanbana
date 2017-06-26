#pragma once

// Enables debug validation layers for dx12/vulkan
#define FAZE_GRAPHICS_VALIDATION_LAYER

#define FAZE_GRAPHICS_EXTRA_INFO // makes everything more verbose

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

#ifdef FAZE_GRAPHICS_EXTRA_INFO
#define GFX_ILOG(msg, ...) F_ILOG("Graphics", msg, ##__VA_ARGS__)
#define GFX_LOG(msg, ...) F_SLOG("Graphics", msg, ##__VA_ARGS__)
#else
#define GFX_ILOG(msg, ...)
#define GFX_LOG(msg, ...)
#endif
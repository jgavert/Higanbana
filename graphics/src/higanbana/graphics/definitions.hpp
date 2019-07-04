#pragma once

// Enables debug validation layers for dx12/vulkan
// slows perf, goes unnoticed, incompatible with nsight in every way on any API supported.
#define HIGANBANA_GRAPHICS_VALIDATION_LAYER

// you can control which implementations are visible with these enums
// point is to use it with Radeon GPU Profiler to avoid it seeing DX12 or viceversa
#define HIGANBANA_GRAPHICS_AD_DX12
#define HIGANBANA_GRAPHICS_AD_VULKAN

//#define HIGANBANA_GRAPHICS_EXTRA_INFO // makes everything more verbose

//#define HIGANBANA_NSIGHT_COMPATIBILITY // disables some colorspace configuration which seems to crash nsight.

// Enables GBV validation for DX12, super slow.
//#define HIGANBANA_GRAPHICS_GPUBASED_VALIDATION

// Could probably convert above to be global booleans instead, so I could toggle them runtime.
// Though many of them require to create all resources from scratch.

namespace higanbana
{
  namespace globalconfig
  {
    namespace graphics
    {
      // barrier booleans, toggle to control. Nothing thread safe here.
      // cannot toggle per window... I guess.
      extern bool GraphicsEnableReadStateCombining;
      extern bool GraphicsEnableHandleCommonState;
      extern bool GraphicsEnableSplitBarriers;
      extern bool GraphicsSplitBarriersPlaceBeginsOnExistingPoints;
    }
  }
}

#ifdef HIGANBANA_GRAPHICS_EXTRA_INFO
#define GFX_ILOG(msg, ...) HIGAN_ILOG("Graphics", msg, ##__VA_ARGS__)
#define GFX_LOG(msg, ...) HIGAN_SLOG("Graphics", msg, ##__VA_ARGS__)
#else
#define GFX_ILOG(msg, ...)
#define GFX_LOG(msg, ...)
#endif
#pragma once

// Enables debug validation layers for dx12/vulkan
// slows perf, goes unnoticed, incompatible with nsight in every way on any API supported.
// #define HIGANBANA_GRAPHICS_VALIDATION_LAYER

//#define HIGANBANA_GRAPHICS_EXTRA_INFO // makes everything more verbose

//#define HIGANBANA_NSIGHT_COMPATIBILITY // disables some colorspace configuration which seems to crash nsight.

// Enables GBV validation for DX12, super slow.
//#define HIGANBANA_GRAPHICS_GPUBASED_VALIDATION

// Could probably convert above to be global booleans instead, so I could toggle them runtime.
// Though many of them require to create all resources from scratch.

#define HIGANBANA_USABLE_SHADER_ARGUMENT_SETS 4
#define HIGANBANA_SHADER_DEBUG_WIDTH 256 
#define HIGANBANA_CONSTANT_BUFFER_AMOUNT 256 * 24 * 64 * 1024
#define HIGANBANA_UPLOAD_MEMORY_AMOUNT 64 * 1024 * 1024

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
      extern int GraphicsHowManyBytesBeforeNewCommandBuffer;
      extern bool GraphicsEnableShaderDebug;
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
#pragma once

#if defined(GFX_VULKAN)
#include "gfxvk/src/Graphics/gfxApi.hpp"
#elif defined(GFX_D3D12)
#include "gfxdx12/src/Graphics/gfxApi.hpp"
#elif defined(GFX_NULL)
#include "gfxnull/src/Graphics/gfxApi.hpp"
#endif


#if defined(GFX_VULKAN)
#include "gfxvk/src/temp.hpp"
#elif defined(GFX_D3D12)
#include "gfxdx12/src/temp.hpp"
#elif defined(GFX_NULL)
#include "gfxnull/src/temp.hpp"
#endif
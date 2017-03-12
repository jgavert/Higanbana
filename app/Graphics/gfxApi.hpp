#pragma once

#if defined(GFX_VULKAN)
#include "faze/src/Graphics/gfxApi.hpp"
#elif defined(GFX_D3D12)
#include "gfxdx12/src/Graphics/gfxApi.hpp"
#elif defined(GFX_NULL)
#include "gfxnull/src/Graphics/gfxApi.hpp"
#endif

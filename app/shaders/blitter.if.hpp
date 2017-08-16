#ifndef BLITTER_HLSL
#define BLITTER_HLSL

#include "app/graphics/definitions.hpp"

FAZE_BEGIN_LAYOUT(Blitter)

struct Configuration 
{
  int colorspace;
};

FAZE_CBUFFER(Configuration)

FAZE_SRV(Buffer<float4>, vertices, 0)
FAZE_SRV(Texture2D<float4>, source, 1)

FAZE_STATIC_SAMPLER(staSam)

FAZE_SRV_TABLE(2)
FAZE_UAV_TABLE(0)

FAZE_END_LAYOUT

#endif /*FAZE_SHADER_DEFINITIONS*/
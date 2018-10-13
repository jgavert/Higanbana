#ifndef MIPMAPPER_HLSL
#define MIPMAPPER_HLSL

#include "definitions.hpp"

FAZE_BEGIN_LAYOUT(Mipmapper)

FAZE_BEGIN_DESCRIPTOR_LAYOUT
FAZE_TEXTURES(1)
FAZE_RWTEXTURES(4)
FAZE_END_DESCRIPTOR_LAYOUT

FAZE_CBUFFER
{
  int mipLevelsToGenerate; // 1-4, with 0 this shader shouldn't be invocated
  int unused;
  float2 texelSize;   // 1.0 / destination dimension
};

FAZE_SRV(Texture2D<float4>, sourceMip0, 0)
FAZE_UAV(RWTexture2D<float4>, mip1, 0)
FAZE_UAV(RWTexture2D<float4>, mip2, 1)
FAZE_UAV(RWTexture2D<float4>, mip3, 2)
FAZE_UAV(RWTexture2D<float4>, mip4, 3)

FAZE_SAMPLER_BILINEAR(bilinearSampler)

FAZE_END_LAYOUT

#endif /*FAZE_SHADER_DEFINITIONS*/
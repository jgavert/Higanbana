#ifndef TRIANGLE_HLSL
#define TRIANGLE_HLSL

#include "app/graphics/definitions.hpp"

FAZE_BEGIN_LAYOUT(Triangle)

struct TriangleConstants
{
  float4 color;
  int colorspace;
};

FAZE_CBUFFER(TriangleConstants)

FAZE_SRV(Buffer<float4>, vertices, 0)
FAZE_SRV(Texture2D<float4>, yellow, 1)

FAZE_STATIC_SAMPLER(staSam)

FAZE_SRV_TABLE(2)
FAZE_UAV_TABLE(0)

FAZE_END_LAYOUT

#endif /*FAZE_SHADER_DEFINITIONS*/
#ifndef TRIANGLE_HLSL
#define TRIANGLE_HLSL

#include "app/graphics/definitions.hpp"

FAZE_BEGIN_LAYOUT(Triangle)

struct TriangleConstants
{
  float4 color;
};

FAZE_CBUFFER(TriangleConstants)

FAZE_SRV_TABLE(0)
FAZE_UAV_TABLE(0)

FAZE_END_LAYOUT

#endif /*FAZE_SHADER_DEFINITIONS*/
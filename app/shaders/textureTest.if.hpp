#ifndef TEXTURETEST_HLSL
#define TEXTURETEST_HLSL

#include "app/graphics/definitions.hpp"

FAZE_BEGIN_LAYOUT(TextureTest)

FAZE_CBUFFER
{
  uint2 iResolution;
  float iTime;
  float iTimeDelta;
  int	iFrame;
  float2 mouse;
  float3 iPos;
  float3 iDir;
  uint _unused;
};

FAZE_UAV(RWTexture2D<float4>, output, 0)

#define FAZE_THREADGROUP_X 8
#define FAZE_THREADGROUP_Y 8
#define FAZE_THREADGROUP_Z 1

FAZE_SRV_TABLE(0)
FAZE_UAV_TABLE(1)

FAZE_END_LAYOUT

#endif /*FAZE_SHADER_DEFINITIONS*/
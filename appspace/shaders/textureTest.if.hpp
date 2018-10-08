#ifndef TEXTURETEST_HLSL
#define TEXTURETEST_HLSL

#include "definitions.hpp"

FAZE_BEGIN_LAYOUT(TextureTest)

FAZE_BEGIN_DESCRIPTOR_LAYOUT
FAZE_RWTEXTURES(1)
FAZE_END_DESCRIPTOR_LAYOUT

FAZE_CBUFFER
{
  uint2 iResolution;
  float iTime;
  float iTimeDelta;
  int	iFrame;
  uint _unused;
  float2 mouse;
  float3 iPos;
  uint _unused2;
  float3 iDir;
  uint _unused3;
  float3 iUpDir;
  uint _unused4;
  float3 iSideDir;
};

FAZE_UAV(RWTexture2D<float4>, output, 0)

FAZE_END_LAYOUT

#endif /*FAZE_SHADER_DEFINITIONS*/
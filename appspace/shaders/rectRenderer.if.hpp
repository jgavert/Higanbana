#ifndef RECTSHADER_HLSL
#define RECTSHADER_HLSL

#include "definitions.hpp"

FAZE_BEGIN_LAYOUT(RectShader)

FAZE_CBUFFER
{
  float4 color;
};

FAZE_SRV(Buffer<float4>, vertices, 0)

FAZE_SRV_TABLE(1)
FAZE_UAV_TABLE(0)

FAZE_END_LAYOUT

#endif /*FAZE_SHADER_DEFINITIONS*/
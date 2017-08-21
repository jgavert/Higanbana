#ifndef IMGUI_HLSL
#define IMGUI_HLSL

#include "app/graphics/definitions.hpp"

FAZE_BEGIN_LAYOUT(ImGui)

FAZE_CBUFFER
{
  float2 reciprocalResolution;
};

FAZE_SRV(Buffer<uint>, vertices, 0)
FAZE_SRV(Texture2D<float>, tex, 1)

FAZE_SAMPLER_POINT(pointSampler)

FAZE_SRV_TABLE(2)
FAZE_UAV_TABLE(0)

FAZE_END_LAYOUT

#endif /*FAZE_SHADER_DEFINITIONS*/
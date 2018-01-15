#ifndef BUFFERTEST_HLSL
#define BUFFERTEST_HLSL

#include "definitions.hpp"

FAZE_BEGIN_LAYOUT(BufferTest)

FAZE_CBUFFER
{
  float _unused;
};

FAZE_SRV(Buffer<float>, input, 0)
FAZE_UAV(RWBuffer<float>, output, 0)

FAZE_SRV_TABLE(1)
FAZE_UAV_TABLE(1)

FAZE_END_LAYOUT

#endif /*FAZE_SHADER_DEFINITIONS*/
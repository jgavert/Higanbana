#include "triangle.if.hpp"

struct PSInput
{
  float2 uv : TEXCOORD0;
  float4 pos : SV_Position;
};

GX_SIGNATURE
float4 PSMain(PSInput input) : SV_Target
{
  return constants.color;
}

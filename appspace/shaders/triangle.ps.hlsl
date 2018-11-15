#include "triangle.if.hpp"

struct PSInput
{
  float2 uv : TEXCOORD0;
  float4 pos : SV_Position;
};

GX_SIGNATURE
float4 main(PSInput input) : SV_Target
{
  float4 final = color;
  final = yellow.SampleLevel(staSam, input.uv, 0);
  //final.xy = input.uv;
  //final.zw = float2(0.f, 0.f);
  return final;
}

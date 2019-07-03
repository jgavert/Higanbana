#include "imgui.if.hpp"

struct PSInput
{
  float4 uv    : TEXCOORD0;
  float4 color : COLOR0;
};

GX_SIGNATURE
float4 main(PSInput i) : SV_Target
{
  float  texColor = tex.Sample(pointSampler, i.uv.xy);
  float4 vertColor = i.color;
  return texColor * vertColor;
}

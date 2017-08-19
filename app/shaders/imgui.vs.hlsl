#include "triangle.if.hpp"

struct PSInput
{
  float4 uv    : TEXCOORD0;
  float4 color : COLOR0;
  float4 pos   : SV_Position;
};

GX_SIGNATURE
PSInput main(uint id: SV_VertexID)
{
  uint loadID = id * 2;

  float4 i1 = vertices.Load(loadID);
  float4 i2 = vertices.Load(loadID + 1);

  VSOutput o;

  float2 fracPos = i1.xy * reciprocalResolution;
  float2 clipPos = lerp(float2(-1, 1), float2(1, -1), fracPos);

  o.uv = float4(i1.zw, 0, 0);
  o.color = i2;
  o.pos = float4(clipPos, 0, 1);

  return o;
}
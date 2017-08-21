#include "imgui.if.hpp"

struct PSInput
{
  float4 uv    : TEXCOORD0;
  float4 color : COLOR0;
  float4 pos   : SV_Position;
};

float4 convertUintToFloat4(uint color)
{
  float a = (1.0f / 255.0f)*float((color >> 24) & 0xff);
  float b = (1.0f / 255.0f)*float((color >> 16) & 0xff);
  float g = (1.0f / 255.0f)*float((color >> 8) & 0xff);
  float r = (1.0f / 255.0f)*float(color & 0xff);
  return float4(r, g, b, a);
}

GX_SIGNATURE
PSInput main(uint id: SV_VertexID)
{
  uint loadID = id * 5;

  float4 i1;
  i1.x = asfloat(vertices.Load(loadID));
  i1.y = asfloat(vertices.Load(loadID + 1));
  i1.z = asfloat(vertices.Load(loadID + 2));
  i1.w = asfloat(vertices.Load(loadID + 3));
  uint col = vertices.Load(loadID + 4);

  PSInput o;

  float2 fracPos = i1.xy * reciprocalResolution;
  float2 clipPos = lerp(float2(-1, 1), float2(1, -1), fracPos);

  o.uv = float4(i1.zw, 0, 0);
  o.color = convertUintToFloat4(col);
  o.pos = float4(clipPos, 0, 1);

  return o;
}
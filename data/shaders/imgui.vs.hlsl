#include "imgui.if.hlsl"
// this is trying to be Vertex shader file.
struct VertexOut
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

[RootSignature(ROOTSIG)]
VertexOut main(uint id: SV_VertexID)
{ 
  uint loadID = id * 5;

  const uint multiplier = 4;

  float4 i1;
  i1 = asfloat(vertices.Load4(loadID*multiplier));
  uint col = vertices.Load((loadID + 4)*multiplier);

  VertexOut o;

  float2 fracPos = i1.xy * constants.reciprocalResolution;
  float2 clipPos = lerp(float2(-1, 1), float2(1, -1), fracPos);

  o.uv = float4(i1.zw, 0, 0);
  o.color = convertUintToFloat4(col);
  o.pos = float4(clipPos, 0, 1);

  return o;
}

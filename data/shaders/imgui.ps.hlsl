#include "imgui.if.hlsl"

struct VertexOut
{
  float4 uv    : TEXCOORD0;
  float4 color : COLOR0;
  float4 pos   : SV_Position;
};

[RootSignature(ROOTSIG)]
float4 main(VertexOut i) : SV_TARGET
{
  float  texColor = tex.Sample(pointSampler, i.uv.xy);
  float4 vertColor = i.color;
  return texColor * vertColor;
}

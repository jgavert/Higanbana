#include "blitter.if.hlsl"

struct PSInput
{
  float2 uv : TEXCOORD0;
  float4 pos : SV_Position;
};

[RootSignature(ROOTSIG)]
PSInput main(uint id: SV_VertexID)
{
  PSInput output;
  float4 data = vertices.Load(id);
  output.pos = float4(data.xy, 0.f, 1.f);
  output.uv = data.zw;
  return output;
}
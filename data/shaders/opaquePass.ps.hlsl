#include "opaquePass.if.hlsl"
// this is trying to be Pixel shader file.
struct VertexOut {   float2 uv : TEXCOORD0;   float4 pos : SV_Position; };

[RootSignature(ROOTSIG)]
float4 main(VertexOut input) : SV_TARGET
{
  return float4(input.pos.zzz*1000.f-0.05f, 1.f);
}

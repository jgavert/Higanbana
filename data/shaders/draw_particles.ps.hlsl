#include "draw_particles.if.hlsl"
// this is trying to be Pixel shader file.
struct VertexOut {   float2 uv : TEXCOORD0;   float4 pos : SV_Position; };

[RootSignature(ROOTSIG)]
float4 main(VertexOut input) : SV_TARGET
{
  return float4(input.uv * input.pos.xy * 0.001, 1, 0.5f);
}

#include "draw_particles.if.hlsl"
// this is trying to be Pixel shader file.
struct VertexOut {   float2 uv : TEXCOORD0;   float4 pos : SV_Position; };

[RootSignature(ROOTSIG)]
float4 main(VertexOut input) : SV_TARGET
{
  float dist = distance(float2(0.5, 0.5), input.uv)*7;
  return float4(1, 0, 0, 1-dist);
}

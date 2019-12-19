#include "opaquePass.if.hlsl"
// this is trying to be Pixel shader file.
struct VertexOut {   float2 uv : TEXCOORD0; float4 normal : NORMAL;  float4 pos : SV_Position; };

[RootSignature(ROOTSIG)]
float4 main(VertexOut input) : SV_TARGET
{
  float2 duv = float2(ddx(input.normal.x), ddy(input.normal.y));
  //return float4(duv*4.f, 0.f, 1.f);
  //return float4(input.uv, 0.f, 1.f);
  float4 normal = float4(float4(duv.x, duv.y, 0, 1)*0.5f+0.5f);
  float3 lightDir = float3(1, 1, 0);
  //lightDir.x = cos(constants.time*0.7f)*0.3f+0.4f;
  //lightDir.y = sin(constants.time*0.7f)*0.3f+0.4f;

  float light = max(0, dot(normal.xyz, lightDir));
  return normal * (light + 0.1f);
  //return float4(input.pos.zzz*1000.f-0.05f, 1.f);
}

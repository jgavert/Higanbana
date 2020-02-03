#include "testMeshShaders.if.hlsl"
// this is trying to be Pixel shader file.
struct VertexOut
{
  float2 uv : TEXCOORD0;
  float3 normal : NORMAL;
  float4 pos : SV_Position;
};

[RootSignature(ROOTSIG)]
float4 main(VertexOut input) : SV_TARGET
{
  float3 color = input.normal;
  //color.xy *= input.uv*0.5;
  return float4(color,1);
}

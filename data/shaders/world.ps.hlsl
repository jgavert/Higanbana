#include "world.if.hlsl"
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
  uint albedo = materials[constants.material].albedoIndex;
  float4 color = float4(0,0,0,1);
  if (albedo > 0) 
    color = materialTextures[albedo-1].SampleLevel(bilinearSampler, input.uv, 0); //input.normal;
  //color.xy *= input.uv*0.5;
  //return float4(input.uv, 0, 1);
  return color;
}

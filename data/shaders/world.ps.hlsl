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
  uint materialIndex = constants.material;
  float4 color = float4(0,0,0,1);
  color.xy = input.uv;
  //if (materialIndex > 0)
  {
    uint albedo = materials[constants.material].albedoIndex;
    
    //input.uv.x = 1.f - input.uv.x;
    //input.uv.y = 1.f - input.uv.y;
    if (albedo > 0) 
      color = materialTextures[albedo-1].SampleLevel(bilinearSampler, input.uv, 0); //input.normal;
  }
  //return float4(input.uv, 0, 1);
  return color;
}

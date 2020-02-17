#include "blit.if.hlsl"

struct VertexOut { 
 float4 uv : TEXCOORD0; 
//float4 pos : SV_Position;
};

[RootSignature(ROOTSIG)]
float4 main(VertexOut vsInput) : SV_TARGET
{
  //return float4(vsInput.uv.x, vsInput.uv.y, 0, 1);
  //return float4(vsInput.uv, 0, 1);
  return texInput.Sample(bilinearSampler, vsInput.uv.xy); 
  //return float4(vsInput.uv, 0, 1);
}

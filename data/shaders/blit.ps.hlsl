#include "blit.if.hlsl"
// this is trying to be Pixel shader file.
#ifdef FAZE_VULKAN
#define VK_LOCATION(index) [[vk::location(index)]]
#else // FAZE_DX12
#define VK_LOCATION(index) 
#endif

struct VertexOut { 
 VK_LOCATION(0) float4 uv : TEXCOORD0; 
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

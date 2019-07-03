#include "blit.if.hlsl"

#ifdef HIGANBANA_VULKAN
#define VK_LOCATION(index) [[vk::location(index)]]
#else // HIGANBANA_DX12
#define VK_LOCATION(index) 
#endif

// this is trying to be Vertex shader file.
struct VertexOut
{
  VK_LOCATION(0) float4 uv : TEXCOORD0;
  VK_LOCATION(1) float4 pos : SV_Position;
};

[RootSignature(ROOTSIG)]
VertexOut main(uint id: SV_VertexID) 
{ 
  VertexOut vtxOut;
  uint id2 = id*2*4;
  vtxOut.pos.x = asfloat(vertexInput.Load(id2));
  vtxOut.pos.y = asfloat(vertexInput.Load(id2+4));
  vtxOut.pos.z = 0.f;
  vtxOut.pos.w = 1.f;
  //vtxOut.uv.x = vtxOut.pos.x;
  //vtxOut.uv.y = vtxOut.pos.y;
  vtxOut.uv.x = (id % 3 == 2) ?  2.f : 0.f;
  vtxOut.uv.y = (id % 3 == 1) ?  2.f : 0.f;
  return vtxOut;
}

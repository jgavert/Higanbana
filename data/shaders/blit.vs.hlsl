#include "blit.if.hlsl"

// this is trying to be Vertex shader file.
struct VertexOut
{
  float4 uv : TEXCOORD0;
  float4 pos : SV_Position;
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

#include "Triangle.if.hlsl"
// this is trying to be Vertex shader file.
struct VertexOut
{
  float2 uv : TEXCOORD0;
  float4 pos : SV_Position;
};


[RootSignature(ROOTSIG)]
VertexOut main(uint id: SV_VertexID)
{ 
  VertexOut vtxOut;
  vtxOut.pos.x = (id % 3 == 2) ?  0.9 : -0.9;
  vtxOut.pos.y = (id % 3 == 1) ?  0.9 : -0.9;
  vtxOut.pos.zw = float2(0,1);
  vtxOut.uv.x = (id % 3 == 2) ?  1 : 0;
  vtxOut.uv.y = (id % 3 == 1) ?  1 : 0;
  return vtxOut;
}

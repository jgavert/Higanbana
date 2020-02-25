#include "world.if.hlsl"
// this is trying to be Vertex shader file.
struct VertexOut
{
  float2 uv : TEXCOORD0;
  float3 normal : NORMAL;
  float4 pos : SV_Position;
};


[RootSignature(ROOTSIG)]
VertexOut main(uint id: SV_VertexID)
{ 
  VertexOut vtxOut;
  vtxOut.pos.xyz = vertices.Load(id);
  vtxOut.pos.w = 1.f;
  CameraSettings settings = cameras.Load(constants.camera);
  vtxOut.pos = mul(vtxOut.pos, settings.perspective);
  vtxOut.normal = normals.Load(id);
  vtxOut.uv = uvs.Load(id);
  //vtxOut.uv.x = (id % 3 == 2) ?  1 : 0;
  //vtxOut.uv.y = (id % 3 == 1) ?  1 : 0;
  return vtxOut;
}

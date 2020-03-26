#include "world.if.hlsl"
// this is trying to be Vertex shader file.
struct VertexOut
{
  float2 uv : TEXCOORD0;
  float3 normal : NORMAL;
  float3 tangent : TANGENT;
  float4 wPos : VIEWSPACEPOS; 
  float4 pos : SV_Position;
};


[RootSignature(ROOTSIG)]
VertexOut main(uint id: SV_VertexID)
{ 
  VertexOut vtxOut;
  float3 inputPos = vertices.Load(id);
  vtxOut.pos.xyz = inputPos;
  vtxOut.pos.w = 1.f;
  CameraSettings settings = cameras.Load(constants.camera);
  vtxOut.pos = mul(vtxOut.pos, settings.perspective);
  vtxOut.normal = normals.Load(id);
  vtxOut.tangent = tangents.Load(id);
  vtxOut.uv = uvs.Load(id);
  vtxOut.wPos = float4(inputPos.xyz, 1.f);
  return vtxOut;
}

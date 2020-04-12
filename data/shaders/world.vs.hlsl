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
  float4 inputPos = float4(vertices.Load(id),1);

  CameraSettings settings = cameras.Load(constants.camera);
  float4 vpos = mul(inputPos, constants.worldMatrix);
  vtxOut.wPos = vpos;
  vpos = mul(vpos, settings.perspective);
  vtxOut.pos = vpos;
  vtxOut.normal = normals.Load(id);
  vtxOut.tangent = tangents.Load(id);
  vtxOut.uv = uvs.Load(id);
  return vtxOut;
}

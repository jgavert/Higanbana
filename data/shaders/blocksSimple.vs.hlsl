#include "blocksSimple.if.hlsl"
// this is trying to be Vertex shader file.
struct VertexOut
{
  float2 uv : TEXCOORD0;
  float4 wPos : VIEWSPACEPOS; 
  float4 pos : SV_Position;
};

float2 faceToUV(in int3 pos, uint face) {
  float2 uv = float2(0,0);
  if (face == 0)
    uv = pos.yz*float2(0.25, 0.33)+float2(0,0.33);
  if (face == 1)
    uv = pos.xz*float2(0.25, 0.33)+float2(0.25, 0.33);
  if (face == 2)
    uv = pos.yz*float2(0.25, 0.33)+float2(0.5, 0.33);
  if (face == 3)
    uv = pos.xz*float2(0.25, 0.33)+float2(0.75, 0.33);
  if (face == 4)
    uv = pos.xy*float2(0.25, 0.33)+float2(0.25, 0.66);
  if (face == 5)
    uv = pos.xy*float2(0.25, 0.33)+float2(0.25, 0);
  return uv;
}


[RootSignature(ROOTSIG)]
VertexOut main(uint id: SV_VertexID)
{ 
  uint face = 0;
  if ((id & 15) > 0) face = uint(id >>4);

  //print(face);
  id = id & 15;

  uint loadID = id * 1;

  const uint multiplier = 4;

  //int3 ipos = vertexInput.Load3(loadID*multiplier);
  uint compressed = vertexInput.Load(loadID*multiplier);
  int3 ipos = int3(compressed>>2, (compressed&2)>>1, compressed&1);
  float2 uv = faceToUV(ipos, face);
  float3 pos = float3(ipos);
  pos = pos *2.f -1.f;
  //float2 uv = asfloat(vertexInput.Load2((loadID + 3)*multiplier));
  CameraSettings settings = cameras.Load(constants.camera.current);

  VertexOut vtxOut;
  float4 bpos = float4(pos, 1.f);

  float4 vpos = bpos + float4(constants.position.xyz * 2, 0.f);
  //vpos.w = 1.f;
  //print(vpos);
  vtxOut.wPos = vpos;
  vpos = mul(vpos, settings.perspective);
  vtxOut.pos = vpos;
  vtxOut.uv = uv;

  return vtxOut;
}

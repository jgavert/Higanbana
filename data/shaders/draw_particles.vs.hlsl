#include "draw_particles.if.hlsl"
// this is trying to be Vertex shader file.
struct VertexOut
{
  float2 uv : TEXCOORD0;
  float4 pos : SV_Position;
};


[RootSignature(ROOTSIG)]
VertexOut main(uint id: SV_VertexID, uint iid: SV_InstanceID)
{ 
  float4 pos = particles[iid].pos;
  CameraSettings settings = cameras.Load(0);

  pos = mul(pos, settings.perspective);
  float4 cameraDir = normalize(mul(float4(0, 0, 1, 1), settings.perspective));
  //if (iid == 0 && id == 1)
  //  print(pos);

  VertexOut vtxOut;
  float2 dirCorner = float2(0,0);
  int2 choice = int2(0, 1);
  if (id < 3)
  {
    choice = int2(1, 0);
  }
  dirCorner.x = (id % 3 == 2) ?  choice.x : choice.y;
  dirCorner.y = (id % 3 == 1) ?  choice.x : choice.y;
  pos.xy += dirCorner;
  vtxOut.pos = pos;
  //vtxOut.pos.z = 0.1;
  //vtxOut.pos.w = 1.f;
  vtxOut.uv.x = (id % 3 == 2) ?  1 : 0;
  vtxOut.uv.y = (id % 3 == 1) ?  1 : 0;

  return vtxOut;
}

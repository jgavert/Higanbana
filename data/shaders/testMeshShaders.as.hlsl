#include "testMeshShaders.if.hlsl"

struct payloadStruct
{ 
  float4x4 perspective; 
}; 
 
groupshared payloadStruct p; 

[RootSignature(ROOTSIG)]
[numthreads(1,1,1)] 
void main(in uint3 groupID : SV_GroupID)    
{ 
  CameraSettings settings = cameras.Load(0);
  p.perspective = settings.perspective;

  DispatchMesh(constants.meshletCount,1,1,p);
}
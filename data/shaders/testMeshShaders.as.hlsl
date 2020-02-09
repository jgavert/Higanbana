#include "testMeshShaders.if.hlsl"
#include "testMesh_payload.hlsl"
 
groupshared payloadStruct p; 

[RootSignature(ROOTSIG)]
[numthreads(1,1,1)] 
void main(
  uint gtid : SV_GroupThreadID,
  in uint3 groupID : SV_GroupID)    
{ 
  CameraSettings settings = cameras.Load(0);
  p.perspective = settings.perspective;

  uint survivingMeshlets = constants.meshletCount;

  DispatchMesh(survivingMeshlets,1,1,p);
}
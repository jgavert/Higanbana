#include "testMeshShaders.if.hlsl"

struct payloadStruct
{ 
    uint myArbitraryData; 
}; 
 
groupshared payloadStruct p; 

[RootSignature(ROOTSIG)]
[numthreads(1,1,1)] 
void main(in uint3 groupID : SV_GroupID)    
{ 
  p.myArbitraryData = groupID.z; 
  DispatchMesh(1,1,1,p);
}
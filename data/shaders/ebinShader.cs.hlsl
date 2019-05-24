#include "ebinShader.if.hlsl"
// this is trying to be Compute shader file.


[RootSignature(ROOTSIG)]
[numthreads(FAZE_THREADGROUP_X, FAZE_THREADGROUP_Y, FAZE_THREADGROUP_Z)]
void main(uint2 id : SV_DispatchThreadID, uint2 gid : SV_GroupThreadID)
{ 


 }

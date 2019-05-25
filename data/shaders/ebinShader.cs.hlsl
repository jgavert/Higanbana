#include "ebinShader.if.hlsl"
// this is trying to be Compute shader file.

[RootSignature(ROOTSIG)]
[numthreads(FAZE_THREADGROUP_X, FAZE_THREADGROUP_Y, FAZE_THREADGROUP_Z)] // @nolint
void main(uint2 id : SV_DispatchThreadID, uint2 gid : SV_GroupThreadID)
{ 
  float val = test2.Load(id.x);
  testOut[id.x] = float4(val, val, val, val);
}

#include "simpleEffect.if.hlsl"
// this is trying to be Compute shader file.


[RootSignature(ROOTSIG)]
[numthreads(HIGANBANA_THREADGROUP_X, HIGANBANA_THREADGROUP_Y, HIGANBANA_THREADGROUP_Z)] // @nolint
void main(uint2 id : SV_DispatchThreadID, uint2 gid : SV_GroupThreadID)
{ 
  const float sx = float(id.x) / constants.resx; 
  const float sy = float(id.y) / constants.resy; 
  output[id] = float4(sx*0.6f, sy*0.6f, 0.5f, 1.f);
}

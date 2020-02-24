#include "tsaa.if.hlsl"
// this is trying to be Compute shader file.


[RootSignature(ROOTSIG)]
[numthreads(HIGANBANA_THREADGROUP_X, HIGANBANA_THREADGROUP_Y, HIGANBANA_THREADGROUP_Z)] // @nolint
void main(uint2 id : SV_DispatchThreadID, uint2 gid : SV_GroupThreadID)
{ 
  float2 uv = float2(float(id.x) / float(constants.outputSize.x), float(id.y) / float(constants.outputSize.y));
  result[id] = source.SampleLevel(bilinearSampler, uv, 0);
}

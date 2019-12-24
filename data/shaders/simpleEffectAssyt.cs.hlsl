#include "simpleEffectAssyt.if.hlsl"
// this is trying to be Compute shader file.

[RootSignature(ROOTSIG)]
[numthreads(HIGANBANA_THREADGROUP_X, HIGANBANA_THREADGROUP_Y, HIGANBANA_THREADGROUP_Z)] // @nolint
void main(uint2 id : SV_DispatchThreadID, uint2 gid : SV_GroupThreadID)
{ 
  float2 uv = float2(id.x/constants.resx, id.y/constants.resy);
  //uv.x = cos(uv.x + constants.time)*0.5+0.5;
  //uv.y = sin(uv.y + constants.time)*0.5+0.5;

  //if (id.x == 0 && id.y == 0)
  //	print(uint(10));
  output[id] = float4(0.05f, 0.05f, 0.2f, 1.f); 
}

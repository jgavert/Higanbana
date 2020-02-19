#include "particles.if.hlsl"
// this is trying to be Compute shader file.


[RootSignature(ROOTSIG)]
[numthreads(HIGANBANA_THREADGROUP_X, HIGANBANA_THREADGROUP_Y, HIGANBANA_THREADGROUP_Z)] // @nolint
void main(uint2 id : SV_DispatchThreadID, uint2 gid : SV_GroupThreadID)
{ 
  if(id.x >= constants.maxParticles)
    return;

  Particle p = previous[id.x];
  if(p.velocity.a > 0.0) {
    p.pos = p.pos + float4(p.velocity.xyz, 0.0)*constants.frameTimeDiff;
    p.velocity.a = p.velocity.a - (0.001*constants.frameTimeDiff);
  } else {
    p.pos = constants.originPoint;
    p.velocity.a = 1.0;
  }
  current[id.x] = p;
}

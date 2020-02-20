#include "particles.if.hlsl"
// this is trying to be Compute shader file.
float nrand(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

[RootSignature(ROOTSIG)]
[numthreads(HIGANBANA_THREADGROUP_X, HIGANBANA_THREADGROUP_Y, HIGANBANA_THREADGROUP_Z)] // @nolint
void main(uint2 id : SV_DispatchThreadID, uint2 gid : SV_GroupThreadID)
{ 
  if(id.x >= constants.maxParticles)
    return;

  Particle p = previous[id.x];
  if(p.velocity.a > 0.0) {
    p.pos = p.pos + float4(p.velocity.xyz, 0.0)*constants.frameTimeDiff*0.1;
    p.velocity.a = p.velocity.a - (0.001*constants.frameTimeDiff*0.1);
  } else {
    p.pos = constants.originPoint;
    
    p.velocity.x = nrand(p.pos.yx + float2(id.x, gid.x) + p.velocity.xy) - 0.5;
    p.velocity.z = nrand(p.pos.zw + float2(id.x, gid.x) + p.velocity.yx) - 0.5;
    p.velocity.y = nrand(p.pos.xy + float2(id.x, gid.x) + p.velocity.xy);
    p.velocity.xyz *= 0.5;
    p.velocity.a = nrand(p.pos.zw + float2(id.x, gid.x) + p.velocity.xy);
  }
  current[id.x] = p;
}

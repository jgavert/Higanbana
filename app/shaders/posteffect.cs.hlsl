#include "posteffect.if.hpp"

CS_SIGNATURE
void main(uint2 id : SV_DispatchThreadID, uint2 gid : SV_GroupThreadID)
{
  float4 val = source[id];
  val.x += 1.f;
  target[id] = val;
}
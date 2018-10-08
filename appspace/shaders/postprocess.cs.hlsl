#include "postprocess.if.hpp"

CS_SIGNATURE
void main(uint2 id : SV_DispatchThreadID, uint2 gid : SV_GroupThreadID)
{
  output[id] = source[id] * luminance[uint2(0,0)];
}
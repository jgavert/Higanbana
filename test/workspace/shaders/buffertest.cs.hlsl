#include "buffertest.if.hpp"

CS_SIGNATURE
void main(uint2 id : SV_DispatchThreadID, uint2 gid : SV_GroupThreadID)
{
  output[id.x] = input[id.x] * 2;
}
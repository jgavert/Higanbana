#include "textureTest.if.hpp"

CS_SIGNATURE
void main(uint id : SV_DispatchThreadID)
{
	if (id >= constants.iResolution.x * constants.iResolution.y)
		return;

	uint2 pos = uint2(0, 0);

	pos.x = id % constants.iResolution.x;
	pos.y = id / constants.iResolution.y;
	
	output[pos] = float4(0.f, 0.4f, sin(constants.iTime), 1.f);
}
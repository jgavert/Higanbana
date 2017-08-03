#include "textureTest.if.hpp"

CS_SIGNATURE
void main(uint id : SV_DispatchThreadID)
{
	if (id >= constants.iResolution.x * constants.iResolution.y)
		return;

	uint2 pos = uint2(0, 0);

	pos.x = id % constants.iResolution.x;
	pos.y = id / constants.iResolution.x;
	
	float2 fp = float2(float(pos.x), float(pos.y));
	fp /= float2(float(constants.iResolution.x), float(constants.iResolution.y));

	output[pos] = float4(sin(constants.iTime)*0.5 + fp.x, cos(constants.iTime)*0.5 + fp.y, 0.f, 1.f);
}
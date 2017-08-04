#include "textureTest.if.hpp"

CS_SIGNATURE
void main(uint2 id : SV_DispatchThreadID)
{
	if (id.x >= constants.iResolution.x || id.y >= constants.iResolution.y)
		return;
	
	float2 fp = float2(float(id.x), float(id.y));
	fp /= float2(float(constants.iResolution.x), float(constants.iResolution.y));

	output[id] = float4(sin(constants.iTime)*0.5 + fp.x, cos(constants.iTime)*0.5 + fp.y, 0.f, 1.f);
}
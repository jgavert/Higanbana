#include "posteffect.if.hpp"

CS_SIGNATURE
void main(uint2 id : SV_DispatchThreadID, uint2 gid : SV_GroupThreadID)
{
	uint column = id.x;

	float4 lastPixel = float4(0,0,0,0);

	if (id.y == 0)
	{
		for (uint y = 0; y < sourceRes.y; ++y)
		{
			uint2 vec = uint2(column, sourceRes.y - y);
			vec.y = min(max(vec.y, 0), sourceRes.y);
	  	float4 val = source[vec];
	  	val *= 0.1f;
	  	val += lastPixel * 0.9f;
	  	//val.x += 0.1f;
	  	lastPixel = val;
	  	target[vec] = val;
	  }
  }
}
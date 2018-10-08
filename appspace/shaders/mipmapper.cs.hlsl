#include "mipmapper.if.hpp"

CS_SIGNATURE
void main(uint2 id : SV_DispatchThreadID, uint2 gid : SV_GroupThreadID)
{
	uint2 idInsideGroup = gid; // use for lds 

    float2 texcoords = texelSize * (id.xy + 0.5);
    float4 color = sourceMip0.SampleLevel(bilinearSampler, texcoords, 0);

	mip1[id] = color;

	if (mipLevelsToGenerate > 1)
	{
		uint2 oID = uint2(id.x / 2, id.y / 2);
		mip2[oID] = float4(0.f, 0.5f, 0.f, 1.f);
	}

	if (mipLevelsToGenerate > 2)
	{
		uint2 oID = uint2(id.x / 4, id.y / 4);
		mip3[oID] = float4(0.f, 0.0f, 0.5f, 1.f);
	}

	if (mipLevelsToGenerate > 3)
	{
		uint2 oID = uint2(id.x / 8, id.y / 8);
		mip4[oID] = float4(0.5f, 0.5f, 0.5f, 1.f);
	}
}
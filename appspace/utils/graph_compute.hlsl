#include "utils/rootSig.hlsl"

ConstantBuffer<consts> consta : register( b0 );
RWTexture2D<float> OutputTex : register( u0 );
// uav missing

[RootSignature(MyRS1)]
[numthreads(32, 1, 1)] // 32 on code side also
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	int2 uv = int2(consta.startUvX,DTid.x);
	if (uv.y >= consta.height)
		return;

	// err figure logic here.
	float4 finalColor = float4(0.4f, 0.1f, 0.1f, 0.6f);
	float pixelHeight = (consta.val - consta.valueMin) / (consta.valueMax - consta.valueMin);
	float drawedPixelHeight = (int)((float)consta.height) * pixelHeight;
	if (drawedPixelHeight == uv.y)
	{
		finalColor = float4(1.f, 1.f, 1.f, 1.f);
	}
	// need to get texture pos, look at min/max if the val is in the same y coordinate, and put it white.
	// otherwise put alpha 0.0 to make it transparent. (I guess)
	OutputTex[uv] = finalColor;
}

#include "utils/rootSig.hlsl"

ConstantBuffer<RootConstants> rootconst : register(b1, space0);
ConstantBuffer<consts> consta : register( b0 );
RWTexture2D<float4> OutputTex[60] : register( u1 );

[RootSignature(MyRS3)]
[numthreads(64, 1, 1)] // 64 on code side also
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	int2 uv = int2(consta.startUvX,DTid.x);
	if (uv.y >= consta.height)
		return;

	// err figure logic here.
	float4 finalColor = float4(0.8f, 0.1f, float(consta.texIndex)*0.1f, 0.4f);
	float pixelHeight = (consta.val - consta.valueMin) / (consta.valueMax - consta.valueMin);
	float drawedPixelHeight = (int)((float)consta.height) * pixelHeight;
	if (abs(drawedPixelHeight - uv.y) < 1.8)
	{
		finalColor = float4(1.f, 1.f, 1.f, 0.6f);
	}
	else if (abs(drawedPixelHeight - uv.y) < 3.5)
	{
		finalColor = float4(0.9f, 0.5f, 0.5f, 0.6f);
	}
	// need to get texture pos, look at min/max if the val is in the same y coordinate, and put it white.
	// otherwise put alpha 0.0 to make it transparent. (I guess)

	OutputTex[0][uv] = finalColor;
	OutputTex[rootconst.texIndex][uv] = finalColor;
	//uint index = consta.texIndex;
	//if (index != 0)
	//	index = 0;
	//OutputTex[index][uv] = finalColor;
	//OutputTex[NonUniformResourceIndex(index)][uv] = finalColor;
}

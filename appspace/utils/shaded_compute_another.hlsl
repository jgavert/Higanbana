#include "utils/rootSig2.hlsl"

ConstantBuffer<RootConstants> rootconst : register(b1, space0);
ConstantBuffer<consts> consta : register( b0 );
RWTexture2D<float4> OutputTex[] : register( u1 );

[RootSignature(MyRS3)]
[numthreads(8, 8, 1)] // 64 on code side also
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	uint2 uv = uint2(DTid.x,DTid.y);
	if (uv.y >= consta.height || uv.x >= consta.width)
		return;

	OutputTex[rootconst.texIndex][uv] = float4(1.f, 0.f, 0.f, 1.f);
}

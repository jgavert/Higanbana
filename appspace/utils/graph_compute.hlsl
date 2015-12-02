#include "utils/rootSig.hlsl"

ConstantBuffer<consts> consta : register(b0);
// uav missing

[RootSignature(MyRS1)]
[numthreads(32, 1, 1)] // 32 on code side also
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	// err figure logic here.
	// need to get texture pos, look at min/max if the val is in the same y coordinate, and put it white.
	// otherwise put alpha 0.0 to make it transparent. (I guess)
}

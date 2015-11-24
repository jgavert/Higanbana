#include "rootSig.hlsl"


StructuredBuffer<buffer> Ind : register(t0);

[RootSignature(MyRS1)]
float4 PSMain(PSInput input) : SV_Target
{
  return float4(input.pos.x, input.pos.y, 0.0f, 1.0f);
}

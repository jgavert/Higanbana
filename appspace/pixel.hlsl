#include "rootSig.hlsl"


StructuredBuffer<buffer> Ind : register(t0);

[RootSignature(MyRS1)]
float4 PSMain(PSInput input) : SV_Target
{
  return float4(input.pos.x/800.0f, input.pos.y/600.f, 0.0f, 1.0f);
}

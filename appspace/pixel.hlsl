#include "rootSig.hlsl"


StructuredBuffer<buffer> Ind : register(t0);

[RootSignature(MyRS1)]
float4 PSMain(PSInput input) : SV_Target
{
  return float4(1.f, 0.f, 0.0f, 1.0f);
}

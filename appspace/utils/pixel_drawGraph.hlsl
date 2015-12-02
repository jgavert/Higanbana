#include "utils/rootSig.hlsl"

ConstantBuffer<consts> consta : register(b0);

[RootSignature(MyRS1)]
float4 PSMain(PSInput input) : SV_Target
{
  // need to sample the UAV from coordinates
  return float4(1.f, 1.f, 0.0f, 1.0f);
}

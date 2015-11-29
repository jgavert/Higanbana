#include "rootSig.hlsl"


[RootSignature(MyRS1)]
float4 PSMain(PSInput input) : SV_Target
{
  return float4(input.pos.x/800.0f, input.pos.y/600.f, 0.0f, 1.0f);
}

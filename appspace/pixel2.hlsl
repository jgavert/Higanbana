#include "rootSig.hlsl"

ConstantBuffer<Constants> consta : register(b0);

[RootSignature(MyRS1)]
float4 PSMain(PSInput input) : SV_Target
{
  return float4(input.pos.x/consta.resolution.x, input.pos.y/consta.resolution.y, sin(consta.time)*0.5+0.5, 1.0f);
  /*float value = input.pos.x/consta.resolution.x/(sin(consta.time*0.2f)+1.0f) + input.pos.y/consta.resolution.y/(sin(consta.time*0.2f)+1.0f);
  return float4(value, value, value, 1.0f);
  */
}

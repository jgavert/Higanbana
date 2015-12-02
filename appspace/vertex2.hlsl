#include "rootSig.hlsl"

ConstantBuffer<Constants> consta : register(b0);
StructuredBuffer<buffer> Ind : register(t0);

[RootSignature(MyRS1)]
PSInput VSMain(uint id: SV_VertexID)
{
  PSInput output;
  output.pos = mul(Ind[id].pos, consta.worldMatrix);
  output.pos = mul(output.pos, consta.viewMatrix);
  output.pos = mul(output.pos, consta.projMatrix);
  return output;
}

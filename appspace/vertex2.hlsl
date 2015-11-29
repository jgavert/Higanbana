#include "rootSig.hlsl"

ConstantBuffer<Constants> consta : register(b0);
StructuredBuffer<buffer> Ind : register(t0);

[RootSignature(MyRS1)]
PSInput VSMain(uint id: SV_VertexID)
{
  PSInput output;

  output.pos = mul(Ind[id].pos, consta.WorldMatrix); // this moved the triangle correctly
  //output.pos = mul(output.pos, consta.ViewMatrix);
  //output.pos = mul(output.pos, consta.ProjectionMatrix);
  return output;
}

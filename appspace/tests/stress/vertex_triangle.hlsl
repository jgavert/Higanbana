#include "tests/stress/rootSig.hlsl"

StructuredBuffer<buffer> Ind : register(t0);

[RootSignature(MyRS1)]
PSInput VSMain(uint id: SV_VertexID, uint inst: SV_InstanceID)
{
  PSInput output;
  output.pos.x = (id == 2) ?  0.01 :  -0.01;
  output.pos.y = (id == 1) ?  0.01 :  -0.01;
  output.pos.zw = float2(0.0,1.0);
  output.pos.xyz += Ind[inst].pos.xyz;
  return output;
}
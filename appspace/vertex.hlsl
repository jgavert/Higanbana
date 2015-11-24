#include "rootSig.hlsl"

StructuredBuffer<buffer> Ind : register(t0);

[RootSignature(MyRS1)]
PSInput VSMain(uint id: SV_VertexID)
{
  PSInput inp;
  inp.pos = Ind[id].pos;
  return inp;
}

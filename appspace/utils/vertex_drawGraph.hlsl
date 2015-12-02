#include "utils/rootSig.hlsl"

ConstantBuffer<consts> consta : register(b0);
// uav missing

[RootSignature(MyRS1)]
PSInput VSMain(uint id: SV_VertexID)
{
  PSInput output;
  // missing quad generation here
  // output uv information also
  output.pos.x = (id == 2) ?  3.0 :  -1.0;
  output.pos.y = (id == 1) ?  3.0 :  -1.0;
  output.pos.zw = float2(1.0,1.0);
  return output;
}

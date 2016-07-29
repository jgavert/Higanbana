#include "utils/rootSig2.hlsl"

ConstantBuffer<consts> consta : register( b0 );

[RootSignature(MyRS3)]
PSInput VSMain(uint id: SV_VertexID)
{
  PSInput output;

  bool x = (id == 2) || (id == 3) || (id == 5); // if -
  bool y = (id == 1) || (id == 4) || (id == 5); // same
  output.pos.x = x ? consta.topleft.x : consta.bottomright.x;
  output.pos.y = y ? consta.bottomright.y : consta.topleft.y;
  output.pos.zw = float2(0.01,1.0);

  // output uv information also
  output.uv = float2(x ? 0.0 : 1.0, y ? 0.0 : 1.0);
  return output;
}

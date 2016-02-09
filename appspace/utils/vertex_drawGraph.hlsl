#include "utils/rootSig.hlsl"

//uint constant : register ( b1 );
ConstantBuffer<consts> consta : register( b0 );
// uav missing

[RootSignature(MyRS3)]
PSInput VSMain(uint id: SV_VertexID)
{
  PSInput output;
  // missing quad generation here, will not cover whole screen.
  bool x = (id == 2) || (id == 3) || (id == 5); // if -
  bool y = (id == 1) || (id == 4) || (id == 5); // same
  output.pos.x = x ? consta.topleft.x : consta.bottomright.x;
  output.pos.y = y ? consta.bottomright.y : consta.topleft.y;
  output.pos.zw = float2(0.01,1.0);
  // output uv information also
  output.uv = float2(x ? -1.0 : 1.0, y ? -1.0 : 1.0);
  return output;
}

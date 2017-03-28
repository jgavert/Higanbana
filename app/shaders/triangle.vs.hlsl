#include "triangle.if.hpp"

struct PSInput
{
  float2 uv : TEXCOORD0;
  float4 pos : SV_Position;
};

GX_SIGNATURE
PSInput VSMain(uint id: SV_VertexID)
{
  PSInput output;
  output.pos.x = (id == 2) ?  3.0 :  -1.0;
  output.pos.y = (id == 1) ?  3.0 :  -1.0;
  output.pos.zw = float2(1.0,1.0);
  return output;
}
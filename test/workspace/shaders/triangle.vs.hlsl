#include "triangle.if.hpp"

struct PSInput
{
  float2 uv : TEXCOORD0;
  float4 pos : SV_Position;
};

GX_SIGNATURE
PSInput main(uint id: SV_VertexID)
{
  PSInput output;
  //output.uv = float2(0,0);
  output.pos = vertices.Load(id);
  output.uv.x = (id == 2) ?  2 : 0; //3.0 :  -1.0;
  output.uv.y = (id == 1) ?  2 : 0; //3.0 :  -1.0;
  //output.pos.zw = float2(1.0,1.0);
  return output;
}
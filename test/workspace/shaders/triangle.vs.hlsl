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
  output.pos = vertices.Load(id);

  id = id % 3;
  output.uv.x = (id == 2) ?  2 : 0;
  output.uv.y = (id == 1) ?  2 : 0;
  return output;
}
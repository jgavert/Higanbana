#include "rectRenderer.if.hpp"

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
  output.uv.x = (id % 3 == 2) ?  1 : 0;
  output.uv.y = (id % 3 == 1) ?  1 : 0;
  return output;
}
#include "blitter.if.hpp"

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
  output.uv = output.pos.zw;
  output.pos.zw = float2(0.f, 1.f);
  return output;
}
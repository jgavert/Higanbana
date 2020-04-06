#include "imgui.if.hlsl"

struct VertexOut
{
  float4 uv    : TEXCOORD0;
  float4 color : COLOR0;
  float4 pos   : SV_Position;
};

[RootSignature(ROOTSIG)]
float4 main(VertexOut i) : SV_TARGET
{
  if (constants.renderCustom > 0) {
    return viewports[constants.renderCustom - 1].Sample(bilinearSampler, i.uv.xy);
  }
  else {
    float  texColor = tex.Sample(pointSampler, i.uv.xy);
    float4 vertColor = i.color;
    return texColor * vertColor;
  }
}

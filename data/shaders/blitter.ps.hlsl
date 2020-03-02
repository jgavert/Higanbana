#include "blitter.if.hlsl"

struct PSInput
{
  float2 uv : TEXCOORD0;
  float4 pos : SV_Position;
};

float4 convertUintToFloat4(uint color)
{
  float a = (1.0f / 255.0f)*float((color >> 24) & 0xff);
  float b = (1.0f / 255.0f)*float((color >> 16) & 0xff);
  float g = (1.0f / 255.0f)*float((color >> 8) & 0xff);
  float r = (1.0f / 255.0f)*float(color & 0xff);
  return float4(r, g, b, a);
}

[RootSignature(ROOTSIG)]
float4 main(PSInput input) : SV_Target
{
  if (constants.intTarget > 0) {
    int2 uv = int2(input.uv * constants.sourceResolution);
    return convertUintToFloat4(sourceInt[uv]);
  }
  return source.SampleLevel(bilinearSampler, input.uv, 0);
}
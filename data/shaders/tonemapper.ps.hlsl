#include "tonemapper.if.hlsl"
#include "lib/color.hlsl"
#include "lib/aces.hlsl"
// this is trying to be Pixel shader file.
struct VertexOut {   float2 uv : TEXCOORD0;   float4 pos : SV_Position; };

[RootSignature(ROOTSIG)]
float4 main(VertexOut input) : SV_TARGET
{
  float4 color = source.SampleLevel(bilinearSampler, input.uv, 0);
  float3 tonemap = color.rgb;//
  //tonemap = rgbToYCoCg(tonemap);
  //tonemap.z = 0;
  //tonemap = yCoCgToRgb(tonemap);
  //tonemap = ACESFitted(tonemap);
  return float4(tonemap, 1.f);
  //return color;
}

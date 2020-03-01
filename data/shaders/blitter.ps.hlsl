#include "blitter.if.hlsl"

struct PSInput
{
  float2 uv : TEXCOORD0;
  float4 pos : SV_Position;
};

[RootSignature(ROOTSIG)]
float4 main(PSInput input) : SV_Target
{
  return source.SampleLevel(bilinearSampler, input.uv, 0);
}
#include "utils/rootSig.hlsl"

ConstantBuffer<consts> consta : register( b0 );
Texture2D<float> tex : register( u0 );
sampler2D smp : register( s0 );

[RootSignature(MyRS1)]
float4 PSMain(PSInput input) : SV_Target
{
  // need to sample the UAV from coordinates
  float2 uv = input.uv;
  // uv.x += (float)consta.startUvX / (float)consta.width
  return tex.Sample(smp, uv);
}

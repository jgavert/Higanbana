#include "utils/rootSig.hlsl"

uint constant : register ( b1 );
ConstantBuffer<consts> consta : register( b0 );
RWTexture2D<float4> tex[63] : register( u1 );

sampler2D smp : register( s0 );

[RootSignature(MyRS1)]
float4 PSMain(PSInput input) : SV_Target
{
  // need to sample the UAV from coordinates
  float2 uv = input.uv;
  uv.x += (float)consta.startUvX / (float)consta.width;
  return float4(tex[constant].Load(int2(uv)).xyz, 0.5);
  //return float4(uv.x, 0.5, 0.5, 0.1);
  //return float4(uv, 0.0, 1.0);
}

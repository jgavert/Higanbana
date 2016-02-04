#include "utils/rootSig.hlsl"

uint constant : register ( b1 );
ConstantBuffer<consts> consta : register( b0 );
RWTexture2D<float4> tex[3] : register( u1 );

SamplerState smp : register( s0 );

[RootSignature(MyRS3)]
float4 PSMain(PSInput input) : SV_Target
{
  // need to sample the UAV from coordinates
  float2 uv = (input.uv);
  uv += float2(1.f, 1.f);
  uv.x += ((float)consta.startUvX) / ((float)consta.width)*2.0;
  if (uv.x >= 2.0)
    uv.x -= 2.0;
  return float4(tex[0].Load(int2(uv*int2(400,100))).xyz, 0.5);

  if (constant == 0)
  	return float4(1.0, 0, 0, 0.1);
  else if (constant == 1 )
    return float4(0.0, 1.0, 0.0, 0.1);
  else if (constant == 2 )
    return float4(0.0,0.0, 1.0, 0.1);
  
  return float4(uv, 0.0, 1.0);
}

#include "utils/rootSig.hlsl"

ConstantBuffer<RootConstants> rootconst : register(b1, space0);
ConstantBuffer<consts> consta : register( b0 );
RWTexture2D<float4> tex[60] : register( u1 );

SamplerState smp : register( s0 );

[RootSignature(MyRS3)]
float4 PSMain(PSInput input) : SV_Target
{
  // need to sample the UAV from coordinates
  float2 uv = (input.uv);
  uv += float2(1.f, 1.f);
  uv.x += ((float)consta.startUvX+1) / ((float)consta.width)*2.0;
  if (uv.x >= 2.0)
    uv.x -= 2.0;


  //return float4(tex[0].Load(int2(uv*int2(consta.width/2.0,consta.height/2.0))).xyzw);
  return float4(tex[consta.texIndex].Load(int2(uv*int2(consta.width/2.0,consta.height/2.0))).xyzw);
  //return float4(tex[NonUniformResourceIndex(consta.texIndex)].Load(int2(uv*int2(consta.width/2.0,consta.height/2.0))).xyzw);
}

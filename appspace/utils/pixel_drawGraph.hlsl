#include "utils/rootSig.hlsl"

ConstantBuffer<RootConstants> rootconst : register(b1, space0);
ConstantBuffer<consts> consta : register( b0 );
Texture2D<float4> tex[60] : register( t1 );

SamplerState smp : register( s0 );

[RootSignature(MyRS3)]
float4 PSMain(PSInput input) : SV_Target
{
  // need to sample the UAV from coordinates
  float2 uv = (input.uv);
  //uv += float2(1.f, 1.f);
  uv.x += ((float)consta.startUvX+1) / ((float)consta.width);
  if (uv.x >= 1.0)
    uv.x -= 1.0;

  return tex[rootconst.texIndex].Sample(smp, uv);
  //return tex[rootconst.texIndex].Load(/*smp,*/ int3(uv*int2(consta.width/4,consta.height/4), 0));
}

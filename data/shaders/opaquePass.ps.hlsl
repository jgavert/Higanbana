#include "opaquePass.if.hlsl"

#include "lib/tonemap_config.hlsl"
#ifdef ACES_ENABLED 
#include "lib/aces12.hlsl"
#include "lib/aces_transforms.hlsl"
#include "lib/aces_odt_adv.hlsl"
#endif
// this is trying to be Pixel shader file.
struct VertexOut {
 float2 uv : TEXCOORD0;
 float4 normal : NORMAL;
 float4 pos : SV_Position;
};

[RootSignature(ROOTSIG)]
float4 main(VertexOut input) : SV_TARGET
{
  // some bullshit math
  float2 duv = float2(ddx(input.normal.x), ddy(input.normal.y));
  float4 normal = float4(float4(duv.x, duv.y, 0, 1)*0.5f+0.5f);
  float3 lightDir = float3(1, 1, 0);

  float light = max(0, dot(normal.xyz, lightDir));
  float4 color = normal*(light+0.1)*constants.color;

#ifdef HIGANBANA_VULKAN
  //return color + float4(0.8,0.0,0.0,0.0);
  //return color + float4(0.0,0.4,0.0,0.0);
#else
  //return color + float4(0.0,0.4,0.0,0.0);
#endif
  //color = float4(1.f, 1.f, 1.f, 0.f) - input.color*8;
  color = constants.color;
#if defined(ACES_ENABLED) && defined(ACEScg_RENDERING) 
  color = invOdtSDR(color);
  color = inverseACESrrt(color);
  color = ACESToACEScg(color);
#endif
  return pow(color, 3)*400;
}

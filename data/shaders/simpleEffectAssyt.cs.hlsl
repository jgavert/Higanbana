#include "simpleEffectAssyt.if.hlsl"

#include "lib/tonemap_config.hlsl"
#ifdef ACES_ENABLED 
#include "lib/aces12.hlsl"
#include "lib/aces_transforms.hlsl"
#include "lib/aces_odt_adv.hlsl"
#endif

[RootSignature(ROOTSIG)]
[numthreads(HIGANBANA_THREADGROUP_X, HIGANBANA_THREADGROUP_Y, HIGANBANA_THREADGROUP_Z)] // @nolint
void main(uint2 id : SV_DispatchThreadID, uint2 gid : SV_GroupThreadID)
{ 
  float2 uv = float2(id.x/constants.resx, id.y/constants.resy);
  //uv.x = cos(uv.x + constants.time)*0.5+0.5;
  //uv.y = sin(uv.y + constants.time)*0.5+0.5;

  //if (id.x == 0 && id.y == 0)
  //	print(uint(10));
  float4 rgbValue = float4(1.f, 1.f, 1.f, 1.f);
#if 0 && defined(ACES_ENABLED) && defined(ACEScg_RENDERING) 
  //rgbValue.rgb = inverseODTRGB(rgbValue.rgb);
  //rgbValue = inverseACESrrt(rgbValue);
  //rgbValue = ACESToACEScg(rgbValue);
#endif
  //rgbValue = sRGBToAcesCg(rgbValue);
  //output[id] = pow(rgbValue*uv.xyyy*3, 3); 
  output[id] = rgbValue*10; 
  //output[id] = float4(0,0,0,0);
}

#include "tonemapper.if.hlsl"

#include "lib/color.hlsl"
#include "lib/aces.hlsl"
#include "lib/tonemap_config.hlsl"
#ifdef ACES_ENABLED
#include "lib/aces12.hlsl"
#include "lib/aces_transforms.hlsl"
#include "lib/aces_odt_adv.hlsl"
#endif
// this is trying to be Pixel shader file.
struct VertexOut {   float2 uv : TEXCOORD0;   float4 pos : SV_Position; };

[RootSignature(ROOTSIG)]
float4 main(VertexOut input) : SV_TARGET
{
  float4 color = source.SampleLevel(bilinearSamplerWarp, input.uv, 0);
#ifdef TEMP_TONEMAP
  //color = saturate(color);
  color = inverse_reinhard(color);
#else
  //color = max(0, color);
#endif
  float4 tonemap = float4(color.rgb, 1);
#ifdef ACES_ENABLED
#ifdef ACEScg_RENDERING
  tonemap = ACEScgToACES(tonemap);
  //tonemap = inverseACESrrt(tonemap);
#else
  //tonemap.rgb = inverseODTRGB(tonemap.rgb); 
  //tonemap = sRGBToAcesCg(tonemap);
  //tonemap = ACEScgToACES(tonemap);
  tonemap = invOdtSDR(tonemap);
  tonemap = inverseACESrrt(tonemap);
#endif
  tonemap = ACESrrt(tonemap);
  
  if (constants.eotf == 1)
    tonemap = odtHDR(tonemap, float3(0.00005, 0.01, 800));
  else
    tonemap = odtSDR(tonemap);

  //float2 modUv = input.uv - float2(0,1);
  //if (all(modUv > 0.2048) && all(modUv < 0.21 ))
  //  print(tonemap);
#else
  //tonemap.rgb = reinhard(tonemap.rgb); 
  //tonemap.rgb = ACESFitted(tonemap.rgb); 
#endif
  return float4(tonemap.rgb, 1.f);
}

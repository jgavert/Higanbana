#include "tonemapper.if.hlsl"

//#define ACES_ENABLED
#ifdef ACES_ENABLED
#include "lib/color.hlsl"
#include "lib/aces.hlsl"
#include "lib/aces12.hlsl"
#include "lib/aces_transforms.hlsl"
#include "lib/aces_odt.hlsl"
#endif
// this is trying to be Pixel shader file.
struct VertexOut {   float2 uv : TEXCOORD0;   float4 pos : SV_Position; };

[RootSignature(ROOTSIG)]
float4 main(VertexOut input) : SV_TARGET
{
  float4 color = source.SampleLevel(bilinearSampler, input.uv, 0);
  //color = mul(color, mul(AP0_2_XYZ_MAT, XYZ_2_AP0_MAT));
#if ACES_ENABLED
  
  color.rgb = inverseODTRGB(color.rgb);
  color = inverseACESrrt(color);
  //color = ACESToACEScg(color);
  //color = ACEScgToACES(color);
  //color = ACESToACEScc(color);
  //color = ACESccToACES(color);
  color = ACESrrt(color);
  color = odtRGB(color);
#endif
  //color.b *= 1000000.f;
  //color = ACEScgToACES(color);
  float2 modUv = input.uv - float2(0,1);
  //if (all(modUv > 0.2048) && all(modUv < 0.206) )
  //  print(color);
  float3 tonemap = color.rgb;//
  //if (tonemap.x > 1.f)
  //  tonemap.x = 0.f;
  //if (tonemap.y > 1.f)
  //  tonemap.y = 0.f;
  //if (tonemap.z > 1.f)
  //  tonemap.z = 0.f;
  //tonemap = rgbToYCoCg(tonemap);
  //tonemap.z = 0;
  //tonemap = yCoCgToRgb(tonemap);
  //tonemap = ACESFitted(tonemap);
  return float4(tonemap, 1.f);
  //return color;
}

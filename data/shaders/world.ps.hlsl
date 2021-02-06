#include "world.if.hlsl"
// this is trying to be Pixel shader file.
#include "lib/tonemap_config.hlsl"
#ifdef ACES_ENABLED 
#include "lib/aces12.hlsl"
#include "lib/aces_transforms.hlsl"
#include "lib/aces_odt_adv.hlsl"
#endif
struct VertexOut
{
  float2 uv : TEXCOORD0;
  float3 normal : NORMAL;
  float3 tangent : TANGENT;
  float4 wPos : VIEWSPACEPOS; 
  float4 pos : SV_Position;
};

struct PixelOut
{
  float4 color: SV_Target0;
  float4 motion: SV_Target1;
};

// taken from microsoft samples
float4 CalcLightingColor(float3 vLightPos, float3 vLightDir, float4 vLightColor, float4 vFalloffs, float3 vPosWorld, float3 vPerPixelNormal)
{
    float3 vLightToPixelUnNormalized = vPosWorld - vLightPos;

    // Dist falloff = 0 at vFalloffs.x, 1 at vFalloffs.x - vFalloffs.y
    float fDist = length(vLightToPixelUnNormalized);

    float fDistFalloff = max(0, (vFalloffs.x - fDist) / vFalloffs.y);

    // Normalize from here on.
    float3 vLightToPixelNormalized = vLightToPixelUnNormalized / fDist;

    // Angle falloff = 0 at vFalloffs.z, 1 at vFalloffs.z - vFalloffs.w
    float fCosAngle = dot(vLightToPixelNormalized, vLightDir / length(vLightDir));
    float fAngleFalloff = (fCosAngle - vFalloffs.z) / vFalloffs.w;

    // Diffuse contribution.
    float fNDotL = -dot(vLightToPixelNormalized, vPerPixelNormal);

    // Ignore angle falloff for a point light.
    fAngleFalloff = 1.0f;  

    return vLightColor * fNDotL * fDistFalloff * fAngleFalloff;
}


[RootSignature(ROOTSIG)]
PixelOut main(VertexOut input) 
{
  uint materialIndex = constants.material;
  float4 albedo = float4(1,1,1,1);
  float3 normal = input.normal;
  float3 pixelNormal = input.normal;
#if defined(ACES_ENABLED) && defined(ACEScg_RENDERING) 
  const float refWhite = ACESToACEScg(inverseACESrrt(invOdtSDR(float4(1,1,1,1)))).x;
#else
  const float refWhite = 1.f;
#endif
  if (materialIndex > 0)
  {
    MaterialData mat = materials[constants.material-1];
    uint albedoId = mat.albedoIndex;
    if (albedoId > 0) { 
      albedo = materialTextures[albedoId-1].SampleLevel(bilinearSamplerWarp, input.uv, 0);
#if defined(ACES_ENABLED) && defined(ACEScg_RENDERING) 
      //albedo = pow(albedo, 2.2);
      albedo = sRGBToAcesCg(albedo);
      //print(sRGBToAcesCg(float4(0.9,0.3,0,0)));
      //albedo = invOdtSDR(albedo);
      //albedo = inverseACESrrt(albedo);
      //albedo = ACESToACEScg(albedo);
      //print(albedo);
      albedo = albedo / refWhite;
      //print(albedo);
      //albedo = albedo / 8;
#endif
    }

    uint normalId = mat.normalIndex;
    if (normalId > 0)
    {
      normal = (float3)materialTextures[normalId-1].SampleLevel(bilinearSamplerWarp, input.uv, 0);
      float3 vNormal = normalize(input.normal);
      float3 vTangent = normalize(input.tangent);
      float3 vBinormal = normalize(cross(vTangent, vNormal));
      float3x3 mTangentSpaceToWorldSpace = float3x3(vTangent, vBinormal, vNormal); 
      float3 pixelNormal = mul(normal, mTangentSpaceToWorldSpace);
    }
  }

  float4 totalLight = float4(1,1,1,1);
#if defined(ACES_ENABLED) && defined(ACEScg_RENDERING) 
  totalLight = sRGBToAcesCg(totalLight);
#endif
  totalLight = totalLight * refWhite * 0.1;

  float4 lColor = float4(1,167.f/255.f,87.f/255.f,1);
  //lColor = float4(1,1,1,1);
#if defined(ACES_ENABLED) && defined(ACEScg_RENDERING) 
  //albedo = pow(albedo, 2.2);
  lColor = sRGBToAcesCg(lColor);
#endif
  float lStr = 1.f;
  float3 lPos = float3(0,0,0);//float3(90,100,110);
  // what does this mean that I have to completely reverse my camera pos...
  lPos = cameras[constants.camera].position.xyz;

  // lightpass
  float4 lightPass = float4(0,0,0,0);
  lightPass = CalcLightingColor(lPos, float3(0,1,0), lColor, float4(500, 400,0.1,1), input.wPos.xyz, pixelNormal);


  totalLight += lightPass * refWhite;

  PixelOut output;
  output.color = albedo * totalLight;
  //output.color = albedo * refWhite;
  CameraSettings previousCamera = cameras.Load(constants.prevCamera);
  float4 prevPos = mul(float4(input.wPos.xyz,1), previousCamera.perspective);
  float4 curPos = input.pos;
  curPos.xy /=  constants.outputSize;
  curPos.xy -= float2(0.5, 0.5);
  curPos.xy *= float2(2,-2);
  curPos.w = 1.f;
  prevPos = prevPos / prevPos.w;
  output.motion = prevPos - curPos; 
  output.motion.xy *= float2(1, -1);
  //output.motion.xy /= constants.outputSize;

  int2 pixel = int2(1000, 500);
  if (input.pos.x > pixel.x && input.pos.x < pixel.x+1 && input.pos.y > pixel.y && input.pos.y < pixel.y+1)
  {
    if (curPos.z > 0.0001)
    {
      //print(float4(curPos.xy, prevPos.xy));
      //output.color = float4(1,1,1,1);
    }
    //print(curPos);
    //float2 wtf = float2(1,1)-prevPos.xy/prevPos.w*float2(-1,1);
    //print(float4(diff));
    //output.motion = float4(1,1,1,1);
  }
  //output.motion = prevPos.zzzz*10; // so we need the previous wPos somehow... previous camera the least.

  //if (albedo.w < 0.00001)
  //  discard;
  return output;
}

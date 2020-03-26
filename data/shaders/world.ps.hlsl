#include "world.if.hlsl"
// this is trying to be Pixel shader file.
struct VertexOut
{
  float2 uv : TEXCOORD0;
  float3 normal : NORMAL;
  float3 tangent : TANGENT;
  float4 wPos : VIEWSPACEPOS; 
  float4 pos : SV_Position;
};

// taken from microsoft samples
float4 CalcLightingColor(float3 vLightPos, float3 vLightDir, float4 vLightColor, float4 vFalloffs, float3 vPosWorld, float3 vPerPixelNormal)
{
    float3 vLightToPixelUnNormalized = vPosWorld - vLightPos;

    // Dist falloff = 0 at vFalloffs.x, 1 at vFalloffs.x - vFalloffs.y
    float fDist = length(vLightToPixelUnNormalized);

    float fDistFalloff = saturate((vFalloffs.x - fDist) / vFalloffs.y);

    // Normalize from here on.
    float3 vLightToPixelNormalized = vLightToPixelUnNormalized / fDist;

    // Angle falloff = 0 at vFalloffs.z, 1 at vFalloffs.z - vFalloffs.w
    float fCosAngle = dot(vLightToPixelNormalized, vLightDir / length(vLightDir));
    float fAngleFalloff = saturate((fCosAngle - vFalloffs.z) / vFalloffs.w);

    // Diffuse contribution.
    float fNDotL = saturate(-dot(vLightToPixelNormalized, vPerPixelNormal));

    // Ignore angle falloff for a point light.
    fAngleFalloff = 1.0f;  

    return vLightColor * fNDotL * fDistFalloff * fAngleFalloff;
}


[RootSignature(ROOTSIG)]
float4 main(VertexOut input) : SV_TARGET
{
  uint materialIndex = constants.material;
  float4 albedo = float4(1,1,1,1);
  float3 normal = float3(1,1,0);
  float3 pixelNormal = input.normal;
  if (materialIndex > 0)
  {
    MaterialData mat = materials[constants.material];
    uint albedoId = mat.albedoIndex;
    if (albedoId > 0) 
      albedo = materialTextures[albedoId-1].SampleLevel(bilinearSampler, input.uv, 0);

    uint normalId = mat.normalIndex;
    if (normalId > 0)
    {
      normal = (float3)materialTextures[normalId-1].SampleLevel(bilinearSampler, input.uv, 0);
      float3 vNormal = normalize(input.normal);
      float3 vTangent = normalize(input.tangent);
      float3 vBinormal = normalize(cross(vTangent, vNormal));
      float3x3 mTangentSpaceToWorldSpace = float3x3(vTangent, vBinormal, vNormal); 
      float3 pixelNormal = mul(normal, mTangentSpaceToWorldSpace);
    }
  }
  float4 totalLight = float4(1,1,1,1) * 0.1;

  float4 lColor = float4(1,1,1,1);
  float lStr = 5.f;
  float3 lPos = float3(0,0,0);//float3(90,100,110);
  // what does this mean that I have to completely reverse my camera pos...
  lPos = cameras[constants.camera].position.xyz * float3(-1,-1,-1);

  // lightpass
  float4 lightPass = float4(0,0,0,0);
  lightPass = CalcLightingColor(lPos, float3(0,1,0), lColor, float4(150, 10,0.5,1), input.wPos.xyz, pixelNormal);

  if (albedo.w < 0.00001)
    discard;

  totalLight += lightPass;

  return albedo * saturate(totalLight);
}

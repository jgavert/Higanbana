#include "testMeshShaders.if.hlsl"
// this is trying to be Pixel shader file.
struct VertexOut
{
  uint id : COLOR0;
  float2 uv : TEXCOORD0;
  float3 normal : NORMAL;
  float4 pos : SV_Position;
};

float3 hsvToRgb(float3 input)
{
  float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
  float3 p = abs(frac(input.xxx + K.xyz) * 6.0 - K.www);

  return input.z * lerp(K.xxx, clamp(p - K.xxx, 0.0, 1.0), input.y);
}

[RootSignature(ROOTSIG)]
float4 main(VertexOut input) : SV_TARGET
{
  float3 hsv = float3((input.id * 4) / float(constants.meshletCount), 1, 1);
  float3 color = input.normal;
  //color.xy *= input.uv*0.5;
  color = hsvToRgb(hsv);
  return float4(color,1);
}

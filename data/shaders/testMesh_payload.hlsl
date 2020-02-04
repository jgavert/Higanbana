struct payloadStruct
{ 
  float4x4 perspective; 
};

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
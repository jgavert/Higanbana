#include "Triangle.if.hlsl"
// this is trying to be Pixel shader file.
struct VertexOut
{
  float2 uv : TEXCOORD0;
  float4 pos : SV_Position;
};

[RootSignature(ROOTSIG)]
float4 main(VertexOut input) : SV_TARGET
{
  int laneIndex = WaveGetLaneIndex();
  float laneThing = float(laneIndex) / 64;
  return float4(input.uv.xyx * laneThing, 1);
  //return float4(0, 1, 0, 1);
}


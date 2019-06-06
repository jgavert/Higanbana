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
  float laneThing = (float(laneIndex)%16)/16;
  float4 color = float4(input.uv.xyx, 1);
  float4 other = QuadReadAcrossDiagonal(color);
  return (color + other)/2;
  //return float4(0, 1, 0, 1);
}

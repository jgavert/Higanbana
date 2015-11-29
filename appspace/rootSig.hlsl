#define MyRS1 "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
			  "CBV(b0), " \
              "SRV(t0), " \
              "UAV(u0) "

struct Constants
{
  float4x4 ProjectionMatrix;
  float4x4 ViewMatrix;
  float4x4 WorldMatrix;
  float time;
  float2 resolution;
  float filler;
};

struct buffer
{
  float4 pos;
};

struct PSInput
{
  float4 pos : SV_Position;
};

#define MyRS1 "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
        "RootConstants(num32BitConstants=1, b1), " \
        "CBV(b0), " \
        "SRV(t0), " \
        "UAV(u0), " \
        "DescriptorTable( SRV(t1, numDescriptors = 3)), " \
        "DescriptorTable( UAV(u1, numDescriptors = 3))"
struct Constants
{
  float4x4 worldMatrix;
  float4x4 viewMatrix;
  float4x4 projMatrix;
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


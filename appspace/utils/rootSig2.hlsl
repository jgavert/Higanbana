#define MyRS3 "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
        "RootConstants(num32BitConstants=1, b1), " \
        "CBV(b0), " \
        "SRV(t0), " \
        "UAV(u0), " \
        "DescriptorTable( SRV(t1, numDescriptors = 60)), " \
        "DescriptorTable( UAV(u1, numDescriptors = 60)), " \
        "StaticSampler(s0, " \
             "addressU = TEXTURE_ADDRESS_WRAP, " \
             "filter = FILTER_MIN_MAG_MIP_LINEAR )"

// need sampler !

struct consts // reflects hlsl
{
  float2 topleft;
  float2 bottomright;
  uint width;
  uint height;
  uint texIndex;
  float empty;
};

struct PSInput
{
  float2 uv : TEXCOORD0;
  float4 pos : SV_Position;
};

struct RootConstants
{
  uint texIndex;
};

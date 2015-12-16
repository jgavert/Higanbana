#define MyRS1 "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
        "RootConstants(num32BitConstants=1, b1), " \
        "CBV(b0), " \
        "SRV(t0), " \
        "UAV(u0), " \
        "DescriptorTable( SRV(t1, numDescriptors = 31)), " \
        "DescriptorTable( UAV(u1, numDescriptors = 31)) " 
//        "StaticSampler(s0, " \
//             "addressU = TEXTURE_ADDRESS_WRAP, " \
//             "filter = FILTER_MIN_MAG_MIP_LINEAR )"

// need sampler !

struct consts // reflects hlsl
{
  float2 topleft;
  float2 bottomright;
  float val;
  float valueMin;
  float valueMax;
  int startUvX;
  int width;
  int height;
  float2 empty;
};

struct PSInput
{
  float2 uv : TEXCOORD0;
  float4 pos : SV_Position;
};

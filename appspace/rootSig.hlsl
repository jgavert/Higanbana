#define MyRS1 "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
              "SRV(t0), " \
              "UAV(u0) "

struct buffer
{
  float4 pos;
};

struct PSInput
{
  float4 pos : SV_Position;
};

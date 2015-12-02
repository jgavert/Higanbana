#define MyRS1 "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | " \
                         "DENY_VERTEX_SHADER_ROOT_ACCESS), " \
              "RootConstants(num32BitConstants=4, b0), " \
              "CBV(b1), " \
              "SRV(t0), " \
              "SRV(t1), " \
              "UAV(u0), " \
              "StaticSampler(s0)," \
              "StaticSampler(s1, " \
                             "addressU = TEXTURE_ADDRESS_CLAMP, " \
                             "filter = FILTER_MIN_MAG_MIP_LINEAR )"


struct buffer
{
  int i;
  int k;
};

StructuredBuffer<buffer> Ind : register(t0);
RWStructuredBuffer<buffer> Outd : register(u0);

[RootSignature(MyRS1)]
[numthreads(50, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
  int tid = DTid.x;
  Outd[tid].i = Ind[tid].i + Ind[tid].k; // Some data actions
  Outd[tid].k = Ind[tid].i - Ind[tid].k;
}
#define MyRS1 "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | " \
                         "DENY_VERTEX_SHADER_ROOT_ACCESS), " \
              "SRV(t0), " \
              "UAV(u0) "


struct buffer
{
  int i;
  int k;
  int x;
  int y;
};

StructuredBuffer<buffer> Ind : register(t0);
RWStructuredBuffer<buffer> Outd : register(u0);

[RootSignature(MyRS1)]
[numthreads(50, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
  uint tid = DTid.x;
  Outd[tid].i = Ind[tid].i + Ind[tid].k; // Some data actions
  Outd[tid].k = tid;
}

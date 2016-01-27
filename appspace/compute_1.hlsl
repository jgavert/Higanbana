#include "rootSig.hlsl"

struct buffer2
{
  int i;
  int k;
  int x;
  int y;
};

StructuredBuffer<buffer2> Ind : register(t0);
RWStructuredBuffer<buffer2> Outd : register(u0);

[RootSignature(MyRS1)]
[numthreads(64, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
  uint tid = DTid.x;
  Outd[tid].i = Ind[tid].i + Ind[tid].k; // Some data actions
  Outd[tid].k = tid;
}

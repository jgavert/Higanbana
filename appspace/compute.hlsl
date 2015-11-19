
struct buffer
{
  int i;
  int k;
  int x; 
  int y;
};

StructuredBuffer<buffer> Ind : register(t0);
RWStructuredBuffer<buffer> Outd : register(u0);

[numthreads(50, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
  int tid = DTid.x;
  Outd[tid].i = Ind[tid].i + Ind[tid].k; // Some data actions
  Outd[tid].k = tid;
}

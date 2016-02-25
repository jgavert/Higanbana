#include "tests/rootSig.hlsl"

struct buffer
{
  uint i;
  uint k;
  uint x;
  uint y;
};

RWStructuredBuffer<buffer> Outd[] : register( u0 );

[RootSignature(MyRS4)]
[numthreads(32, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	int uv = DTid.x;
	buffer buf;
	buf.i = uv;
	buf.k = uv;
	buf.x = uv;
	buf.y = uv;
	Outd[NonUniformResourceIndex(uv)][0] = buf;
}

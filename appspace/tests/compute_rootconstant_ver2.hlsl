#include "tests/rootSig.hlsl"

struct buffer
{
  uint i;
  uint k;
  uint x;
  uint y;
};

ConstantBuffer<RootConstants> rootconst : register(b1, space0);
RWStructuredBuffer<buffer> Outd[60] : register( u1 );

[RootSignature(MyRS4)]
[numthreads(1, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	int uv = DTid.x;
	buffer buf;
	buf.i = 0;
	buf.k = 0;
	buf.x = 0;
	buf.y = 0;
	buf.i = uv;
	buf.k = rootconst.texIndex;
	Outd[rootconst.texIndex][uv] = buf;
}

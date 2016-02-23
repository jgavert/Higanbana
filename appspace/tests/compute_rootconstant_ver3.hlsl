#include "tests/rootSig.hlsl"

struct buffer
{
  uint i;
  uint k;
  uint x;
  uint y;
};

ConstantBuffer<RootConstants> rootconst : register(b1, space0);
StructuredBuffer<buffer> Inbuf[] : register( t1 );
RWStructuredBuffer<buffer> Outd : register( u0 );

[RootSignature(MyRS4)]
[numthreads(1, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	int uv = DTid.x;
	buffer buf;
	const buffer input = Inbuf[rootconst.texIndex].Load(0);
	buf.i = input.i;
	buf.k = input.k;
	buf.x = input.x;
	buf.y = rootconst.texIndex;
	Outd[0] = buf;
}

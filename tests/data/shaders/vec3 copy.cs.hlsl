#include "vec3 copy.if.hlsl"
// this is trying to be Compute shader file.


[RootSignature(ROOTSIG)]
[numthreads(HIGANBANA_THREADGROUP_X, HIGANBANA_THREADGROUP_Y, HIGANBANA_THREADGROUP_Z)] // @nolint
void main(uint2 id : SV_DispatchThreadID, uint2 gid : SV_GroupThreadID)
{ 
	if (id.x == 0 && id.y == 0)
	{
		float3 i0 = input[0];
		float3 i1 = input[1];
		float3 r = mul(i1, i0);
		result[0] = r.x;
		result[1] = r.y;
		result[2] = r.z;
		result[6] = i0.x;
		result[7] = i0.y;
		result[8] = i0.z;
		result[9] = i1.x;
		result[10] = i1.y;
		result[11] = i1.z;
		i0 = constants.vec1;
		i1 = constants.vec2;
		r = mul(i1, i0);
		result[3] = r.x;
		result[4] = r.y;
		result[5] = r.z;


	}
}

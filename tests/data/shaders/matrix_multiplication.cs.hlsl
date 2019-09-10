#include "matrix_multiplication.if.hlsl"
// this is trying to be Compute shader file.


[RootSignature(ROOTSIG)]
[numthreads(HIGANBANA_THREADGROUP_X, HIGANBANA_THREADGROUP_Y, HIGANBANA_THREADGROUP_Z)] // @nolint
void main(uint2 id : SV_DispatchThreadID, uint2 gid : SV_GroupThreadID)
{
  float4x4 res = mul(constants.matr1, constants.matr2);
  result[0] = res._m00;
  result[1] = res._m01;
  result[2] = res._m02;
  result[3] = res._m03;
  result[4] = res._m10;
  result[5] = res._m11;
  result[6] = res._m12;
  result[7] = res._m13;
  result[8] = res._m20;
  result[9] = res._m21;
  result[10] = res._m22;
  result[11] = res._m23;
  result[12] = res._m30;
  result[13] = res._m31;
  result[14] = res._m32;
  result[15] = res._m33;
}

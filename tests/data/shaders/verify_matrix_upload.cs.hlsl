#include "verify_matrix_upload.if.hlsl"
// this is trying to be Compute shader file.


[RootSignature(ROOTSIG)]
[numthreads(HIGANBANA_THREADGROUP_X, HIGANBANA_THREADGROUP_Y, HIGANBANA_THREADGROUP_Z)] // @nolint
void main()
{ 
  diff[0] = constants.matr._m00;
  diff[1] = constants.matr._m01;
  //diff[1] = 5.f;
  diff[2] = constants.matr._m02;
  diff[3] = constants.matr._m03;
  diff[4] = constants.matr._m10;
  diff[5] = constants.matr._m11;
  diff[6] = constants.matr._m12;
  diff[7] = constants.matr._m13;
  diff[8] = constants.matr._m20;
  diff[9] = constants.matr._m21;
  diff[10] = constants.matr._m22;
  diff[11] = constants.matr._m23;
  diff[12] = constants.matr._m30;
  diff[13] = constants.matr._m31;
  diff[14] = constants.matr._m32;
  diff[15] = constants.matr._m33;

  diff[16] = constants.row1.x;
  diff[17] = constants.row1.y;
  diff[18] = constants.row1.z;
  diff[19] = constants.row1.w;
  diff[20] = constants.row2.x;
  diff[21] = constants.row2.y;
  diff[22] = constants.row2.z;
  diff[23] = constants.row2.w;
  diff[24] = constants.row3.x;
  diff[25] = constants.row3.y;
  diff[26] = constants.row3.z;
  diff[27] = constants.row3.w;
  diff[28] = constants.row4.x;
  diff[29] = constants.row4.y;
  diff[30] = constants.row4.z;
  diff[31] = constants.row4.w;

}

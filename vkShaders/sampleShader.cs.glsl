#include "sampleShader.if.hpp"

layout (local_size_x = 32, local_size_y = 1, local_size_z = 1) in;
void main()
{
  uint gid = gl_GlobalInvocationID.x+pConstants.member1+pConstants.member2;
  float a = dataIn[gid].element;
  dataOut[gid].element = a + 0.5;
}

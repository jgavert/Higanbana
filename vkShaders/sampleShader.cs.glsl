#include "sampleShader.if.hpp"

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;
void main()
{
  uint gid = gl_GlobalInvocationID.x;
  if (gid >= 100)
  	return;
  float a = dataIn[gid].element;
  dataOut[gid].element = 5.5;
}


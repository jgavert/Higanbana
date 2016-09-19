#include "shader_defines.h"

layout(std430, binding = 0) buffer DataIn
{
  float a[];
} dataIn;

layout(std430, binding = 1) buffer DataOut
{
  float a[];
} dataOut;

layout (local_size_x = 32, local_size_y = 1, local_size_z = 1) in;
void main()
{
  uint gid = gl_GlobalInvocationID.x;
  float a = dataIn.a[gid];
  dataOut.a[gid] = a + 0.5;
}

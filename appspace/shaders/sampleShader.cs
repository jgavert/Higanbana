#version 450

//#extension GL_ARB_compute_shader : enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

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

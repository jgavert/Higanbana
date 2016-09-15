#version 450
#extension GL_ARB_compute_shader : enable

layout(set = 0, binding = 0, std430) readonly buffer DataIn
{
    float a[];
} dataIn;

layout(set = 0, binding = 1, std430) buffer DataOut
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
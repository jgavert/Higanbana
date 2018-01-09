#include "posteffect.if.hpp"

groupshared float groupPixels[FAZE_THREADGROUP_X*3*2];

int pixelIndex(int threadID)
{
	return threadID * 4;
}

void storePixel(int threadID, float4 val)
{
	int ourIndex = pixelIndex(threadID);
	groupPixels[ourIndex] = val.x;
  groupPixels[ourIndex+1] = val.y;
  groupPixels[ourIndex+2] = val.z;
}

float4 getPixel(int threadID)
{
	int ourIndex = pixelIndex(threadID);
	float4 val2 = float4(0,0,0,1);
  val2.x = groupPixels[ourIndex];
  val2.y = groupPixels[ourIndex + 1];
  val2.z = groupPixels[ourIndex + 2];
  return val2;
}

CS_SIGNATURE
void main(uint2 id : SV_DispatchThreadID, uint2 gid : SV_GroupThreadID)
{
	uint column = id.x / FAZE_THREADGROUP_X;

  int allY = sourceRes.y / FAZE_THREADGROUP_X;
  //allY /= 2;
  allY *= 2;
 
	for (uint y = 0; y < allY; y++)
	{

	  // first load to lds

		uint2 vec = uint2(column, y * FAZE_THREADGROUP_X + gid.x*2);
  	storePixel(gid.x*2+0, source[vec]);
  	vec.y += 1;
  	storePixel(gid.x*2+1, source[vec]);

  	GroupMemoryBarrier();

  	float4 res = float4(0,0,0,1);
  	int samples = FAZE_THREADGROUP_X/4;
  	float multi = 1.f / samples;
  	for (int f = 0; f < samples; ++f)
  	{
  		res += getPixel(gid.x + f) * multi;
  	}
  	
  	uint2 vec2 = uint2(column, y * FAZE_THREADGROUP_X + gid.x);

  	target[vec2] = res;
  }
}
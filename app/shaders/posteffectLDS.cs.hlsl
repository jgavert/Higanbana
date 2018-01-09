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

float cubicPulse( float c, float w, float x )
{
    x = abs(x - c);
    if( x>w ) return 0.0;
    x /= w;
    return 1.0 - x*x*(3.0-2.0*x);
}

float normpdf(in float x, in float sigma)
{
	return 0.39894*exp(-0.5*x*x/(sigma*sigma))/sigma;
}

#define SAMPLES FAZE_THREADGROUP_X/4


CS_SIGNATURE
void main(uint2 id : SV_DispatchThreadID, uint2 gid : SV_GroupThreadID)
{
	uint column = id.x / FAZE_THREADGROUP_X;

  int allY = sourceRes.y / FAZE_THREADGROUP_X;
  //allY /= 2;
  allY *= 2;
 
  float curve[SAMPLES];
  float Z = 0.f;
  for (int f = 0; f < SAMPLES; ++f)
	{
		curve[f] = normpdf(f, SAMPLES*0.5);
		Z += curve[f];
	}
 
	for (uint y = 0; y < allY; y++)
	{

	  // first load to lds

		uint2 vec = uint2(column, y * FAZE_THREADGROUP_X + gid.x*2);
  	storePixel(gid.x*2+0, source[vec]);
  	vec.y += 1;
  	storePixel(gid.x*2+1, source[vec]);

  	GroupMemoryBarrier();

  	float4 res = float4(0,0,0,1);
  	//float multi = 1.f / SAMPLES;
  	for (int f = 0; f < SAMPLES; ++f)
  	{
  		res += getPixel(gid.x + f) * curve[f];
  	}
  	
  	uint2 vec2 = uint2(column, y * FAZE_THREADGROUP_X + gid.x);

  	target[vec2] = res/(Z);
  }
}
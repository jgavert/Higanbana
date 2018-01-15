#include "posteffect.if.hpp"

#define DEBUG_STRIPES 0
#define DEBUG_SAMPLES 0

#define SAMPLES 10

#if DEBUG_SAMPLES
#define SAMPLES 64
#endif

#define READPATTERN 1

// our SAMPLES max is FAZE_THREADGROUP_X, otherwise groupPixels is too small

groupshared float groupPixels[FAZE_THREADGROUP_X*3*2];

int pixelIndex(int threadID)
{
	return threadID *3;
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

CS_SIGNATURE
void main(uint2 id : SV_DispatchThreadID, uint2 gid : SV_GroupThreadID)
{
	uint column = id.x / FAZE_THREADGROUP_X;

  int allY = sourceRes.y / FAZE_THREADGROUP_X;
  //allY /= 2;
  allY *= 2;
 
  float curve[SAMPLES];
  float Z = 0.f;
  const int halfSamples = SAMPLES/2+SAMPLES%2;
  for (int f = 0; f < SAMPLES; ++f)
	{
		curve[f] = pow(normpdf(f-halfSamples, halfSamples), 8);
		Z += curve[f];
	}
 
	for (int y = 0; y < allY; y++)
	{
	  // first load to lds
#if READPATTERN
	  const int gidx = gid.x;
	  const int yoffset = FAZE_THREADGROUP_X;
#else
		const int gidx = gid.x*2;
		const int yoffset = 1;
#endif
    int yyy = y * FAZE_THREADGROUP_X + gidx - 1;
	  yyy = min(max(yyy, 0), sourceRes.y);
		uint2 vec = uint2(column, yyy);
		
    GroupMemoryBarrierWithGroupSync();
#if DEBUG_STRIPES == 1
  	storePixel(gidx+0, source[vec]);
  	vec.y += yoffset;
  	storePixel(gidx+yoffset, source[vec]+float4(0, 0, 0.05*gid.x,0));
#else
  	storePixel(gidx, source[vec]);
  	vec.y += yoffset;
  	storePixel(gidx+yoffset, source[vec]);
#endif

  	GroupMemoryBarrierWithGroupSync();

  	uint2 vec2 = uint2(column, y * FAZE_THREADGROUP_X + gid.x);
#if DEBUG_SAMPLES
		target[vec2] = float4(curve[gid.x]*10,0,0,1) / Z;
#else
  	float4 res = float4(0,0,0,1);
  	for (int f = 0; f < SAMPLES; ++f)
  	{
  		res += getPixel(gid.x + f) * curve[f];
  	}
#if DEBUG_STRIPES == 2
  	target[vec2] = res/Z + float4(0, 0.005*gid.x, 0,1);
#else
		target[vec2] = float4((res/Z).xyz,1);
#endif
#endif
  }
}
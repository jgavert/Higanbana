#include "tsaa.if.hlsl"
// this is trying to be Compute shader file.

float4 sample3by3(float2 uv, float2 pixelSize)
{
  float4 endResult = source.SampleLevel(bilinearSampler, uv+pixelSize, 0);
  endResult += source.SampleLevel(bilinearSampler, uv-pixelSize, 0);
  endResult += source.SampleLevel(bilinearSampler, uv+pixelSize*float2(-1, 1), 0);
  endResult += source.SampleLevel(bilinearSampler, uv+pixelSize*float2(1, -1), 0);
  endResult += source.SampleLevel(bilinearSampler, uv+pixelSize*float2(0, 1), 0);
  endResult += source.SampleLevel(bilinearSampler, uv+pixelSize*float2(1, 0), 0);
  endResult += source.SampleLevel(bilinearSampler, uv+pixelSize*float2(0, -1), 0);
  endResult += source.SampleLevel(bilinearSampler, uv+pixelSize*float2(-1, 0), 0);
  return endResult;
}

[RootSignature(ROOTSIG)]
[numthreads(HIGANBANA_THREADGROUP_X, HIGANBANA_THREADGROUP_Y, HIGANBANA_THREADGROUP_Z)] // @nolint
void main(uint2 id : SV_DispatchThreadID, uint2 gid : SV_GroupThreadID)
{ 
  float historyWeight = 0.9;
  float2 uv = float2(float(id.x) / float(constants.outputSize.x), float(id.y) / float(constants.outputSize.y));
  float2 subsampleSize = float2(1 / float(constants.sourceSize.x*2), 1 / float(constants.sourceSize.y*2));
  //float2 jitterRange = constants.jitter/8.f; // [-1, 1]
  float2 subsamplePos = constants.jitter * (subsampleSize * 0.5); // [-subsample/2, subsample/2]
  float2 subsampleUV = uv + subsamplePos;
  float4 oldPixel = history[id];
  float4 srcPixel = source.SampleLevel(bilinearSampler, subsampleUV, 0);
  float4 current = srcPixel;
  
  float4 boxcolor = sample3by3(subsampleUV, subsampleSize)+srcPixel;
  float distToCurrent = distance(oldPixel*9, boxcolor);

  float filtered = 0.f;
  
  if (distToCurrent > 3)
  {
    //historyWeight /= min(2, distToCurrent);
    filtered = distToCurrent;
  }

  float remainingWeight = 1.f - historyWeight;
  current = current * remainingWeight + oldPixel * historyWeight;
  boxcolor /= 9;
  float4 colorClamp = float4(0.01, 0.01, 0.01, 1);
  current = clamp(current, max(float4(0,0,0,1),boxcolor-colorClamp), min(float4(1,1,1,1), boxcolor+colorClamp));

  //float4 diff = abs(oldPixel - current);
  result[id] = current;
  //float4 gt = source.SampleLevel(bilinearSampler, uv, 0);
  //float diff = distance(oldPixel, srcPixel);
  debug[id] = (oldPixel - current)*4;
  //debug[id] = (oldPixel*9 - (boxcolor+srcPixel));
  //debug[id] = float4(filtered,filtered,filtered,1);
  //debug[id] = (boxcolor+srcPixel) / 9;
}

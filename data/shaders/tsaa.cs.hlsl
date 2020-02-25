#include "tsaa.if.hlsl"
// this is trying to be Compute shader file.


[RootSignature(ROOTSIG)]
[numthreads(HIGANBANA_THREADGROUP_X, HIGANBANA_THREADGROUP_Y, HIGANBANA_THREADGROUP_Z)] // @nolint
void main(uint2 id : SV_DispatchThreadID, uint2 gid : SV_GroupThreadID)
{ 
  float2 uv = float2(float(id.x) / float(constants.outputSize.x), float(id.y) / float(constants.outputSize.y));
  float2 subsampleSize = float2(1 / float(constants.outputSize.x*2), 1 / float(constants.outputSize.y*2));
  //float2 jitterRange = constants.jitter/8.f; // [-1, 1]
  float2 subsamplePos = constants.jitter * subsampleSize * 0.5f; // [-subsample/2, subsample/2]
  float2 subsampleUV = uv + subsamplePos;
  float4 oldPixel = history[id];
  float4 current = source.SampleLevel(bilinearSampler, subsampleUV, 0);
  float distToCurrent = distance(oldPixel, current);
  float historyWeight = 0.9;
  



  float remainingWeight = 1.f - historyWeight;
  current = current * remainingWeight + oldPixel * historyWeight;

  //float4 diff = abs(oldPixel - current);
  result[id] = current;
  //float4 gt = source.SampleLevel(bilinearSampler, uv, 0);
  float diff = distance(current, oldPixel);
  debug[id] = (oldPixel - current)*2;
}

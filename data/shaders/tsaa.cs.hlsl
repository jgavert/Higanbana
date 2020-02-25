#include "tsaa.if.hlsl"
// this is trying to be Compute shader file.


[RootSignature(ROOTSIG)]
[numthreads(HIGANBANA_THREADGROUP_X, HIGANBANA_THREADGROUP_Y, HIGANBANA_THREADGROUP_Z)] // @nolint
void main(uint2 id : SV_DispatchThreadID, uint2 gid : SV_GroupThreadID)
{ 
  float2 uv = float2(float(id.x) / float(constants.outputSize.x), float(id.y) / float(constants.outputSize.y));
  float2 subsampleSize = float2(1 / float(constants.outputSize.x*8), 1 / float(constants.outputSize.y*8));
  float2 subsampleUV = uv-subsampleSize*(constants.jitter/8.f);
  float4 oldPixel = history[id];//.SampleLevel(bilinearSampler, uv, 0);
  float4 current = source.SampleLevel(bilinearSampler, subsampleUV, 0);
  float distToCurrent = distance(oldPixel, current);
  //if (id.x == 100 && id.y == 100)
  {
    //print(subsampleSize*constants.jitter);
  }
  current = (current + oldPixel) * 0.5;

  //float4 diff = abs(oldPixel - current);
  result[id] = current;
  float4 gt = source.SampleLevel(bilinearSampler, uv, 0);
  float diff = distance(oldPixel, gt);
  debug[id] = (oldPixel - gt)*2;
}

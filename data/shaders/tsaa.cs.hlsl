#include "tsaa.if.hlsl"
// this is trying to be Compute shader file.

float4 sample3by3(float2 uv, float2 pixelSize)
{
  float4 endResult = source.SampleLevel(pointSampler, uv+pixelSize, 0);
  endResult += source.SampleLevel(pointSampler, uv-pixelSize, 0);
  endResult += source.SampleLevel(pointSampler, uv+pixelSize*float2(-1, 1), 0);
  endResult += source.SampleLevel(pointSampler, uv+pixelSize*float2(1, -1), 0);
  endResult += source.SampleLevel(pointSampler, uv+pixelSize*float2(0, 1), 0);
  endResult += source.SampleLevel(pointSampler, uv+pixelSize*float2(1, 0), 0);
  endResult += source.SampleLevel(pointSampler, uv+pixelSize*float2(0, -1), 0);
  endResult += source.SampleLevel(pointSampler, uv+pixelSize*float2(-1, 0), 0);
  return endResult;
}

float4 load3by3(int2 uv)
{
  float4 endResult = source[uv + int2(1,1)];
  endResult += source[uv - int2(1,1)];
  endResult += source[uv + int2(-1,1)];
  endResult += source[uv + int2(1,-1)];
  endResult += source[uv + int2(0,1)];
  endResult += source[uv + int2(1,0)];
  endResult += source[uv + int2(0,-1)];
  endResult += source[uv + int2(-1,0)];
  return endResult;
}

[RootSignature(ROOTSIG)]
[numthreads(HIGANBANA_THREADGROUP_X, HIGANBANA_THREADGROUP_Y, HIGANBANA_THREADGROUP_Z)] // @nolint
void main(uint2 id : SV_DispatchThreadID, uint2 gid : SV_GroupThreadID)
{ 
  float historyWeight = 0.90;
  uint zoom = 1;
  float2 offsetUV = float2(0,0);
  float2 sourceSubsampleSize = float2(1 / float(constants.sourceSize.x*2), 1 / float(constants.sourceSize.y*2));

  float2 uv = float2((float(id.x)+0.5) / float(constants.outputSize.x*zoom), (float(id.y)+0.5) / float(constants.outputSize.y*zoom));
  uv += offsetUV;
  float2 uv_jitter_offset = constants.jitter * float2(1,-1) * sourceSubsampleSize;
  int2 sid = int2(((uv+uv_jitter_offset) * constants.sourceSize));
  float4 oldPixel = history[id];
  float4 srcPixel = source[sid]; //.SampleLevel(bilinearSampler, uv+uv_jitter_offset, 0);
  float4 current = srcPixel;
  
  float4 boxcolor = (load3by3(sid)+srcPixel) / 9; //sample3by3(subsampleUV, sourceSubsampleSize)+srcPixel;
  //float distToCurrent = distance(oldPixel, boxcolor);
  float rDist = distance(oldPixel.r, boxcolor.r);
  float bDist = distance(oldPixel.b, boxcolor.b);
  float gDist = distance(oldPixel.g, boxcolor.g);
  float maxDiff = max(max(rDist, bDist), gDist);

  float filtered = 0.f;
  
  //if (maxDiff > 0.3)
  {
    //historyWeight *= 1 - maxDiff*0.25;
    filtered = maxDiff;
  }

  float remainingWeight = 1.f - historyWeight;
  current = current * remainingWeight + oldPixel * historyWeight;
  //boxcolor /= 9;
  //float4 colorClamp = float4(historyWeight/2, historyWeight/2, historyWeight/2, 1);
  //current = clamp(current, max(float4(0,0,0,1),boxcolor-colorClamp), min(float4(1,1,1,1), boxcolor+colorClamp));
  if (0)
  {
    float2 suv = float2((float(sid.x)+0.5) / float(constants.sourceSize.x), (float(sid.y)+0.5) / float(constants.sourceSize.y));
    float2 jittered = suv - uv_jitter_offset;
    float2 dsuv = (jittered-offsetUV)*float2(constants.outputSize*zoom);// + constants.jitter * subsampleSize;
    int2 paintID = int2(dsuv);
    if (paintID.x == id.x && paintID.y == id.y)
    {
      //debug[id] = srcPixel;
      //current = float4(1, 1, 1, 1);
    }
    else
    {
      //debug[id] = float4(0,0,0,0);
    }
  }

  //float4 diff = abs(oldPixel - current);
  result[id] = current;
  //float4 gt = source.SampleLevel(bilinearSampler, uv, 0);
  //float diff = distance(oldPixel, srcPixel);
  //debug[id] = (oldPixel - current)*4;
  //debug[id] = (oldPixel*9 - (boxcolor+srcPixel));
  //historyWeight = 1 - historyWeight;
  //debug[id] = float4(rDist, gDist, bDist,1);
  debug[id] = float4(maxDiff, maxDiff, maxDiff,1);
  //debug[id] = (boxcolor+srcPixel) / 9;
}

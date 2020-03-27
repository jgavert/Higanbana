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

void debug3by3(int2 uv, float4 color)
{
  debug[uv + int2(1,1)] = color;
  debug[uv - int2(1,1)] = color;
  debug[uv + int2(-1,1)] = color;
  debug[uv + int2(1,-1)] = color;
  debug[uv + int2(0,1)] = color;
  debug[uv + int2(1,0)] = color;
  debug[uv + int2(0,-1)] = color;
  debug[uv + int2(-1,0)] = color;
}

int2 convertFromSourceToOutputID(int2 sourceIndex, int zoom, float2 offsetUV, float2 uv_jitter_offset)
{
  float2 suv = float2((float(sourceIndex.x)+0.5) / float(constants.sourceSize.x), (float(sourceIndex.y)+0.5) / float(constants.sourceSize.y));
  float2 jittered = suv - uv_jitter_offset;
  float2 dsuv = (jittered-offsetUV)*float2(constants.outputSize*zoom);// + constants.jitter * subsampleSize;
  int2 paintID = int2(dsuv);
  return sourceIndex;
}

[RootSignature(ROOTSIG)]
[numthreads(HIGANBANA_THREADGROUP_X, HIGANBANA_THREADGROUP_Y, HIGANBANA_THREADGROUP_Z)] // @nolint
void main(uint2 id : SV_DispatchThreadID, uint2 gid : SV_GroupThreadID)
{ 
  float historyWeight = 0.95;
  float zoom = 1.0;
  float2 offsetUV = float2(0.4,0.4)*0;
  float2 outputSubsampleSize = float2(1 / float(constants.outputSize.x*2), 1 / float(constants.outputSize.y*2));
  float2 sourceSubsampleSize = float2(1 / float(constants.sourceSize.x*2), 1 / float(constants.sourceSize.y*2));

  float2 uv = float2((float(id.x)+0.5) / float(constants.outputSize.x*zoom), (float(id.y)+0.5) / float(constants.outputSize.y*zoom));
  uv += offsetUV;
  float2 uv_jitter_offset = constants.jitter * float2(1,-1) * sourceSubsampleSize;
  int2 sid = int2(((uv+uv_jitter_offset) * constants.sourceSize));

  float4 srcPixel = source[sid]; //.SampleLevel(bilinearSampler, uv+uv_jitter_offset, 0);
  float4 current = srcPixel;

  float4 motionVec = motion[sid]*0.5f;
  //motionVec.xy *= float2(0.5,0.5);// + float2(0.5, 0.5); 
  //motionVec /= zoom;
  //motionVec *= 0.02;
  //motionVec = float4(0,0,0,0);
  //motionVec.y *= -1.f;
  float2 correctUV = uv + motionVec.xy;
  float2 huv = (correctUV - offsetUV) * constants.outputSize*zoom;
  int2 hid = int2(huv);
  //float4 oldPixel = history[hid];
  float4 oldPixel = history.SampleLevel(bilinearSampler, correctUV - offsetUV, 0);
  if (uv.y < 0.54 && uv.y > 0.539)
    if (uv.x < 0.522 && uv.x > 0.521)
    {
      //print(hid-id);
      //oldPixel = float4(1,1,1,1);
    }
  
  
  float4 boxcolor = (load3by3(sid)+srcPixel) / 9; //sample3by3(subsampleUV, sourceSubsampleSize)+srcPixel;
  //float distToCurrent = distance(oldPixel, boxcolor);
  float rDist = distance(oldPixel.r, boxcolor.r);
  float bDist = distance(oldPixel.b, boxcolor.b);
  float gDist = distance(oldPixel.g, boxcolor.g);
  float maxDiff = max(max(rDist, bDist), gDist);

  float filtered = 0.f;
  float distToSourcePixel = distance(uv, uv+uv_jitter_offset);
  float sampleInsideOutputPixel = distToSourcePixel - length(outputSubsampleSize);

  if (sampleInsideOutputPixel > 0)
  {
    historyWeight = historyWeight*2;
    filtered = distToSourcePixel*100;
  }
  if (any(correctUV > 1.f) || any(correctUV < 0.f)) {
    historyWeight = 0;
  } 
  
  if (maxDiff > 0.1)
  {
    historyWeight *=  1.f - maxDiff;
    //filtered = maxDiff;
  }

  historyWeight = min(0.95, historyWeight);
  //historyWeight = 1.f;
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
    //debug3by3(paintID, float4(0,1,0,1));
    if (paintID.x == id.x && paintID.y == id.y)
    {
      //debug3by3(sid, float4(0,1,0,1));
      //debug[id] = srcPixel;
      //debug3by3(paintID, float4(0,1,0,1));
      //current = float4(1, 1, 1, 1);
    }
    else
    {
    }
  }

  //float4 diff = abs(oldPixel - current);
  result[id] = current;
  //float4 motionVec = abs(motion[sid]);
  //debug[id] = float4(1,1,1,1);
  float motionLength = length(motionVec)*0.02;

  debug[id] = float4(maxDiff.xxx, 1);
  //debug[id] = float4(motionLength.xx*1000, 0, 1);
  //float4 gt = source.SampleLevel(bilinearSampler, uv, 0);
  //float diff = distance(oldPixel, srcPixel);
  //debug[id] = (oldPixel - current)*4;
  //debug[id] = (oldPixel*9 - (boxcolor+srcPixel));
  //historyWeight = 1 - historyWeight;
  //debug[id] = float4(rDist, gDist, bDist,1);
  //debug[id] = float4(filtered, filtered, filtered,1);
  //debug[id] = float4(maxDiff, maxDiff, maxDiff,1);
  //debug[id] = (boxcolor+srcPixel) / 9;
}

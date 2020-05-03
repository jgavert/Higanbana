#include "tsaa.if.hlsl"

#include "lib/tonemap_config.hlsl"
#include "lib/color.hlsl"

/*
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
}*/

float4 loadFromSource(int2 uv) {
  float4 col = source[uv];
#ifdef TEMP_TONEMAP
  col = max(0, col);
  col = saturate(reinhard(col));
#endif
  return col;
}

float4 sampleFromSource(float2 uv) {
  float4 col = source.SampleLevel(bilinearSampler, uv, 0);
#ifdef TEMP_TONEMAP
  col = reinhard(col);
#endif
  return col;
}

void load3by3(int2 uv, out float3 minC, out float3 maxC)
{
  float3 sample = loadFromSource(uv + int2(1,1)).rgb;
  minC = min(minC, sample);
  maxC = max(maxC, sample);
  sample = loadFromSource(uv - int2(1,1)).rgb;
  minC = min(minC, sample);
  maxC = max(maxC, sample);
  sample = loadFromSource(uv + int2(-1,1)).rgb;
  minC = min(minC, sample);
  maxC = max(maxC, sample);
  sample = loadFromSource(uv + int2(1,-1)).rgb;
  minC = min(minC, sample);
  maxC = max(maxC, sample);
  sample = loadFromSource(uv + int2(0,1)).rgb;
  minC = min(minC, sample);
  maxC = max(maxC, sample);
  sample = loadFromSource(uv + int2(1,0)).rgb;
  minC = min(minC, sample);
  maxC = max(maxC, sample);
  sample = loadFromSource(uv + int2(0,-1)).rgb;
  minC = min(minC, sample);
  maxC = max(maxC, sample);
  sample = loadFromSource(uv + int2(-1,0)).rgb;
  minC = min(minC, sample);
  maxC = max(maxC, sample);
  //minC = sample;
  //maxC = sample;
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
bool isGTthan4(float4 base, float4 compared) {
  return (base.x > compared.x) || (base.y > compared.y) || (base.z > compared.z);
}

bool isGTthan3(float3 base, float3 compared) {
  return (base.x > compared.x) || (base.y > compared.y) || (base.z > compared.z);
}

bool isGTthan2(float2 base, float2 compared) {
  return (base.x > compared.x) || (base.y > compared.y);
}
bool isGTthan2(float2 base, float compared) {
  return (base.x > compared) || (base.y > compared);
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

  float3 srcPixel = loadFromSource(sid).rgb; //.SampleLevel(bilinearSampler, uv+uv_jitter_offset, 0);
  //srcPixel = sampleFromSource(uv+uv_jitter_offset);
  float3 current = srcPixel;

  float3 motionVec = motion[sid].xyz*0.50f;
  //motionVec.xy += constants.jitter * float2(1,-1) * outputSubsampleSize;
  float motLen = length(motionVec);
  //motionVec.xy *= float2(0.5,0.5);// + float2(0.5, 0.5); 
  //motionVec /= zoom;
  //motionVec *= 0.02;
  //motionVec = float4(0,0,0,0);
  //motionVec.y *= -1.f;

  float2 correctUV = uv + motionVec.xy;
  float2 huv = (correctUV - offsetUV) * constants.outputSize*zoom;
  int2 hid = int2(huv);
  //float4 oldPixel = history[hid];
  float2 historyLoc = correctUV - offsetUV;
  float3 oldPixel = history.SampleLevel(bilinearSampler, historyLoc, 0).rgb;
  oldPixel = max(0, oldPixel);

  if (any(historyLoc < 0.f) || any(historyLoc > 1.f))
    historyWeight = 0.05;

  //if (uv.y < 0.54 && uv.y > 0.539)
  //  if (uv.x < 0.522 && uv.x > 0.521)
    {
      //print(hid-id);
      //oldPixel = float4(1,1,1,1);
    }
 
  float3 minC = srcPixel.rgb;
  float3 maxC = srcPixel.rgb;
  
  //load3by3(sid, minC, maxC); //sample3by3(subsampleUV, sourceSubsampleSize)+srcPixel;
  float boxsize = 1.5;
  float3 b00 = source.SampleLevel(bilinearSampler, uv+uv_jitter_offset+sourceSubsampleSize*float2(1, 1)*boxsize, 0).rgb;
  minC = min(minC, b00);
  maxC = max(maxC, b00);
  float3 b01 = source.SampleLevel(bilinearSampler, uv+uv_jitter_offset+sourceSubsampleSize*float2(1, -1)*boxsize, 0).rgb;
  minC = min(minC, b01);
  maxC = max(maxC, b01);
  float3 b10 = source.SampleLevel(bilinearSampler, uv+uv_jitter_offset+sourceSubsampleSize*float2(-1, 1)*boxsize, 0).rgb;
  minC = min(minC, b10);
  maxC = max(maxC, b10);
  float3 b11 = source.SampleLevel(bilinearSampler, uv+uv_jitter_offset+sourceSubsampleSize*float2(-1, -1)*boxsize, 0).rgb;
  minC = min(minC, b11);
  maxC = max(maxC, b11);

  minC = max(0, minC);
  maxC = max(0, maxC);

  //float distToCurrent = distance(oldPixel, boxcolor);
  //float rDist = distance(oldPixel.r, boxcolor.r);
  //float bDist = distance(oldPixel.b, boxcolor.b);
  //float gDist = distance(oldPixel.g, boxcolor.g);
  float maxDiff = distance(minC, maxC);

  float filtered = 0.f;
  float distToSourcePixel = distance(uv, uv+uv_jitter_offset);
  float sampleInsideOutputPixel = distToSourcePixel - length(outputSubsampleSize);

  if (sampleInsideOutputPixel > 0)
  {
    historyWeight = historyWeight*0.95;
    //filtered = distToSourcePixel*100;
  }
  if (any(correctUV > 1.f) || any(correctUV < 0.f)) {
    //historyWeight = 0.1;
  }

  //if (any(oldPixel < minC*0.80) || any(oldPixel > maxC*1.2))
  if (motLen > 0.00001f && (isGTthan3(minC*0.8, oldPixel.rgb) || isGTthan3(oldPixel.rgb, maxC*1.1)))
  {
    historyWeight = historyWeight;
    //oldPixel = float3(1,1,1);
    //filtered = maxDiff;
    //motLen = 0.1f;
  }

  historyWeight = max(0,min(0.99, historyWeight));
  //historyWeight = 0.95;
  //historyWeight = 1.f;
  float remainingWeight = 1.f - historyWeight;
  current = current * remainingWeight + oldPixel * historyWeight;
  

  //boxcolor /= 9;
  //float4 colorClamp = float4(historyWeight/2, historyWeight/2, historyWeight/2, 1);
  //if (motLen > 0.00001f) {
    //current += float3(1,1,1);
    current.rgb = clamp(current.rgb, minC, maxC);
  //}
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
  result[id] = float4(current, 1);//srcPixel;
  //float4 motionVec = abs(motion[sid]);
  //debug[id] = float4(1,1,1,1);
  float motionLength = length(motionVec)*10000;

  //debug[id] = float4(oldPixel, 1);
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

#ifndef __color__
#define __color__

static const float3x3 s_rgbToYCoCg = {
  0.25f, 0.5f, 0.25f,
  0.5f, 0.f, -0.5f,
  -0.25f, 0.5f, -0.25f
};

static const float3x3 s_yCoCgToRgb = {
  1.f, 1.f, -1.f,
  1.f, 0.f, 1.f,
  1.f, -1.f, -1.f
};

float3 rgbToYCoCg(float3 rgb) {
  return mul(rgb, transpose(s_rgbToYCoCg)); 
} 

float3 yCoCgToRgb(float3 ycocg) {
  //float tmp = ycocg.x - ycocg.z;
  //return float3(tmp + ycocg.y, ycocg.x + ycocg.z, tmp - ycocg.y);
  return mul(ycocg, transpose(s_yCoCgToRgb)); 
} 

#endif // __color__
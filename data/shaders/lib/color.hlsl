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

float3 reinhard(float3 rgb) {
  return rgb / (rgb + 1);
}
float4 reinhard(float4 rgb) {
  return float4(rgb.rgb / (rgb.rgb + 1), rgb.w);
}

float3 inverse_reinhard(float3 rgb) {
  return 1.f / (1.f - rgb) - 1.f;
}

float4 inverse_reinhard(float4 rgb) {
  return float4(1.f / (1.f - rgb.rgb) - 1.f, rgb.w);
}

#endif // __color__
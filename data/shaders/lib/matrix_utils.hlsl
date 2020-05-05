#ifndef __matrix_utils__
#define __matrix_utils__

static const float4x4 _identity = { 1.f, 0.f, 0.f, 0.f,
                            0.f, 1.f, 0.f, 0.f,
                            0.f, 0.f, 1.f, 0.f,
                            0.f, 0.f, 0.f, 1.f};

// verified to produce correct result
float4x4 inverse(float4x4 m) {
    float n11 = m[0][0], n12 = m[1][0], n13 = m[2][0], n14 = m[3][0];
    float n21 = m[0][1], n22 = m[1][1], n23 = m[2][1], n24 = m[3][1];
    float n31 = m[0][2], n32 = m[1][2], n33 = m[2][2], n34 = m[3][2];
    float n41 = m[0][3], n42 = m[1][3], n43 = m[2][3], n44 = m[3][3];

    float t11 = n23 * n34 * n42 - n24 * n33 * n42 + n24 * n32 * n43 - n22 * n34 * n43 - n23 * n32 * n44 + n22 * n33 * n44;
    float t12 = n14 * n33 * n42 - n13 * n34 * n42 - n14 * n32 * n43 + n12 * n34 * n43 + n13 * n32 * n44 - n12 * n33 * n44;
    float t13 = n13 * n24 * n42 - n14 * n23 * n42 + n14 * n22 * n43 - n12 * n24 * n43 - n13 * n22 * n44 + n12 * n23 * n44;
    float t14 = n14 * n23 * n32 - n13 * n24 * n32 - n14 * n22 * n33 + n12 * n24 * n33 + n13 * n22 * n34 - n12 * n23 * n34;

    float det = n11 * t11 + n21 * t12 + n31 * t13 + n41 * t14;
    float idet = 1.0f / det;

    float4x4 ret;

    ret[0][0] = t11 * idet;
    ret[0][1] = (n24 * n33 * n41 - n23 * n34 * n41 - n24 * n31 * n43 + n21 * n34 * n43 + n23 * n31 * n44 - n21 * n33 * n44) * idet;
    ret[0][2] = (n22 * n34 * n41 - n24 * n32 * n41 + n24 * n31 * n42 - n21 * n34 * n42 - n22 * n31 * n44 + n21 * n32 * n44) * idet;
    ret[0][3] = (n23 * n32 * n41 - n22 * n33 * n41 - n23 * n31 * n42 + n21 * n33 * n42 + n22 * n31 * n43 - n21 * n32 * n43) * idet;

    ret[1][0] = t12 * idet;
    ret[1][1] = (n13 * n34 * n41 - n14 * n33 * n41 + n14 * n31 * n43 - n11 * n34 * n43 - n13 * n31 * n44 + n11 * n33 * n44) * idet;
    ret[1][2] = (n14 * n32 * n41 - n12 * n34 * n41 - n14 * n31 * n42 + n11 * n34 * n42 + n12 * n31 * n44 - n11 * n32 * n44) * idet;
    ret[1][3] = (n12 * n33 * n41 - n13 * n32 * n41 + n13 * n31 * n42 - n11 * n33 * n42 - n12 * n31 * n43 + n11 * n32 * n43) * idet;

    ret[2][0] = t13 * idet;
    ret[2][1] = (n14 * n23 * n41 - n13 * n24 * n41 - n14 * n21 * n43 + n11 * n24 * n43 + n13 * n21 * n44 - n11 * n23 * n44) * idet;
    ret[2][2] = (n12 * n24 * n41 - n14 * n22 * n41 + n14 * n21 * n42 - n11 * n24 * n42 - n12 * n21 * n44 + n11 * n22 * n44) * idet;
    ret[2][3] = (n13 * n22 * n41 - n12 * n23 * n41 - n13 * n21 * n42 + n11 * n23 * n42 + n12 * n21 * n43 - n11 * n22 * n43) * idet;

    ret[3][0] = t14 * idet;
    ret[3][1] = (n13 * n24 * n31 - n14 * n23 * n31 + n14 * n21 * n33 - n11 * n24 * n33 - n13 * n21 * n34 + n11 * n23 * n34) * idet;
    ret[3][2] = (n14 * n22 * n31 - n12 * n24 * n31 - n14 * n21 * n32 + n11 * n24 * n32 + n12 * n21 * n34 - n11 * n22 * n34) * idet;
    ret[3][3] = (n12 * n23 * n31 - n13 * n22 * n31 + n13 * n21 * n32 - n11 * n23 * n32 - n12 * n21 * n33 + n11 * n22 * n33) * idet;
    return ret;
}

// verified to produce correct result
float3x3 inverse(float3x3 m) {
    float green = m._m00 * m._m11 * m._m22 + m._m10 * m._m21 * m._m02 + m._m20 * m._m01 * m._m12; 
    float red =   m._m02 * m._m11 * m._m20 + m._m12 * m._m21 * m._m00 + m._m22 * m._m01 * m._m10;
    float det = green - red;
    float idet = 1.0f / det;

    float3x3 ret;
    ret._m00 =        (m._m11*m._m22 - m._m12*m._m21);
    ret._m01 = -1.f * (m._m10*m._m22 - m._m12*m._m20);
    ret._m02 =        (m._m10*m._m21 - m._m11*m._m20);

    ret._m10 = -1.f * (m._m01*m._m22 - m._m02*m._m21);
    ret._m11 =        (m._m00*m._m22 - m._m02*m._m20);
    ret._m12 = -1.f * (m._m00*m._m21 - m._m01*m._m20);

    ret._m20 =        (m._m01*m._m12 - m._m02*m._m11);
    ret._m21 = -1.f * (m._m00*m._m12 - m._m02*m._m10);
    ret._m22 =        (m._m00*m._m11 - m._m01*m._m10);
    ret = transpose(ret);
    ret = mul(ret,idet);
    return ret;
}

//#define REVERSE_MATRIX_MUL

float3 mult_f3_f33(float3 input, float3x3 mat) {
#ifdef REVERSE_MATRIX_MUL
  return mul(mat, input);
#else
  return mul(input, mat);
#endif
}

float3 mult_f3_f44(float3 input, float4x4 mat) {
#ifdef REVERSE_MATRIX_MUL
  return mul(mat, float4(input,0)).rgb;
#else
  return mul(float4(input,0), mat).rgb;
#endif
}

float3x3 mult_f33_f33(float3x3 m1, float3x3 m2) {
#ifdef REVERSE_MATRIX_MUL
  return mul(m2, m1);
#else
  return mul(m1, m2);
#endif
}
float4x4 mult_f44_f44(float4x4 m1, float4x4 m2) {
#ifdef REVERSE_MATRIX_MUL
  return mul(m2, m1);
#else
  return mul(m1, m2);
#endif
}

#endif
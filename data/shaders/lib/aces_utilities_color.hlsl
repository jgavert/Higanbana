#ifndef __aces_utilities_color__
#define __aces_utilities_color__

static const float TINY = 0.00001f; //1e-10;

struct Chromaticities
{
  float2 red;
  float2 green;
  float2 blue;
  float2 white;
};

static const Chromaticities AP0 = // ACES Primaries from SMPTE ST2065-1
{
  { 0.73470,  0.26530},
  { 0.00000,  1.00000},
  { 0.00010, -0.07700},
  { 0.32168,  0.33767}
};

static const Chromaticities AP1 = // Working space and rendering primaries for ACES 1.0
{
  { 0.713,    0.293},
  { 0.165,    0.830},
  { 0.128,    0.044},
  { 0.32168,  0.33767}
};

static const Chromaticities REC709_PRI =
{
  { 0.64000,  0.33000},
  { 0.30000,  0.60000},
  { 0.15000,  0.06000},
  { 0.31270,  0.32900}
};

static const Chromaticities P3D60_PRI =
{
  { 0.68000,  0.32000},
  { 0.26500,  0.69000},
  { 0.15000,  0.06000},
  { 0.32168,  0.33767}
};

static const Chromaticities P3D65_PRI =
{
  { 0.68000,  0.32000},
  { 0.26500,  0.69000},
  { 0.15000,  0.06000},
  { 0.31270,  0.32900}
};

static const Chromaticities P3DCI_PRI =
{
  { 0.68000,  0.32000},
  { 0.26500,  0.69000},
  { 0.15000,  0.06000},
  { 0.31400,  0.35100}
};

static const Chromaticities ARRI_ALEXA_WG_PRI =
{
  { 0.68400,  0.31300},
  { 0.22100,  0.84800},
  { 0.08610, -0.10200},
  { 0.31270,  0.32900}
};

static const Chromaticities REC2020_PRI = 
{
  { 0.70800,  0.29200},
  { 0.17000,  0.79700},
  { 0.13100,  0.04600},
  { 0.31270,  0.32900}
};

static const Chromaticities RIMMROMM_PRI = 
{
  { 0.7347,  0.2653},
  { 0.1596,  0.8404},
  { 0.0366,  0.0001},
  { 0.3457,  0.3585}
};

static const Chromaticities SONY_SGAMUT3_PRI =
{
  { 0.730,  0.280},
  { 0.140,  0.855},
  { 0.100, -0.050},
  { 0.3127,  0.3290}
};

static const Chromaticities SONY_SGAMUT3_CINE_PRI =
{
  { 0.766,  0.275},
  { 0.225,  0.800},
  { 0.089, -0.087},
  { 0.3127,  0.3290}
};

static const Chromaticities CANON_CGAMUT_PRI =
{
  { 0.7400,  0.2700},
  { 0.1700,  1.1400},
  { 0.0800, -0.1000},
  { 0.3127,  0.3290}
};

static const Chromaticities RED_WIDEGAMUTRGB_PRI =
{
  { 0.780308,  0.304253},
  { 0.121595,  1.493994},
  { 0.095612, -0.084589},
  { 0.3127,  0.3290}
};

static const Chromaticities PANASONIC_VGAMUT_PRI =
{
  { 0.730,  0.280},
  { 0.165,  0.840},
  { 0.100, -0.030},
  { 0.3127,  0.3290}
};

/* ---- Conversion Functions ---- */
// Various transformations between color encodings and data representations
//

// Transformations between CIE XYZ tristimulus values and CIE x,y 
// chromaticity coordinates
float3 XYZ_2_xyY( float3 XYZ)
{  
  //static const float TINY = 0.00001f; //1e-10;
  float3 xyY;
  float divisor = (XYZ[0] + XYZ[1] + XYZ[2]);
  if (divisor == 0.) divisor = TINY;

  xyY[0] = XYZ[0] / divisor;
  xyY[1] = XYZ[1] / divisor;  
  xyY[2] = XYZ[1];

  return xyY;
}

float3 xyY_2_XYZ( float3 xyY)
{
  float3 XYZ;
  XYZ[0] = xyY[0] * xyY[2] / max( xyY[1], TINY);
  XYZ[1] = xyY[2];  
  XYZ[2] = (1.0 - xyY[0] - xyY[1]) * xyY[2] / max( xyY[1], TINY);

  return XYZ;
}


// Transformations from RGB to other color representations

/* ---- Chromatic Adaptation ---- */

static const float3x3 CONE_RESP_MAT_BRADFORD = {
   0.89510, -0.75020,  0.03890,
   0.26640,  1.71350, -0.06850,
  -0.16140,  0.03670,  1.02960
};

static const float3x3 CONE_RESP_MAT_CAT02 = {
   0.73280, -0.70360,  0.00300,
   0.42960,  1.69750,  0.01360,
  -0.16240,  0.00610,  0.98340
};

float3x3 calculate_cat_matrix(
    float2 src_xy,         // x,y chromaticity of source white
    float2 des_xy,         // x,y chromaticity of destination white
    float3x3 coneRespMat = CONE_RESP_MAT_BRADFORD
  )
{
  //
  // Calculates and returns a 3x3 Von Kries chromatic adaptation transform 
  // from src_xy to des_xy using the cone response primaries defined 
  // by coneRespMat. By default, coneRespMat is set to CONE_RESP_MAT_BRADFORD. 
  // The default coneRespMat can be overridden at runtime. 
  //

  const float3 src_xyY = { src_xy[0], src_xy[1], 1. };
  const float3 des_xyY = { des_xy[0], des_xy[1], 1. };

  float3 src_XYZ = xyY_2_XYZ( src_xyY );
  float3 des_XYZ = xyY_2_XYZ( des_xyY );

  float3 src_coneResp = mult_f3_f33( src_XYZ, coneRespMat);
  float3 des_coneResp = mult_f3_f33( des_XYZ, coneRespMat);

  float3x3 vkMat = {
      des_coneResp[0] / src_coneResp[0], 0.0, 0.0,
      0.0, des_coneResp[1] / src_coneResp[1], 0.0,
      0.0, 0.0, des_coneResp[2] / src_coneResp[2]
  };

  float3x3 cat_matrix = mult_f33_f33( coneRespMat, mult_f33_f33( vkMat, inverse( coneRespMat ) ) );

  return cat_matrix;
}



/* ---- Signal encode/decode functions ---- */

float moncurve_f( float x, float gamma, float offs ) {
  // Forward monitor curve
  float y;
  const float fs = (( gamma - 1.0) / offs) * pow( offs * gamma / ( ( gamma - 1.0) * ( 1.0 + offs)), gamma);
  const float xb = offs / ( gamma - 1.0);
  if ( x >= xb) 
    y = pow( ( x + offs) / ( 1.0 + offs), gamma);
  else
    y = x * fs;
  return y;
}

float moncurve_r( float y, float gamma, float offs )
{
  // Reverse monitor curve
  float x;
  const float yb = pow( offs * gamma / ( ( gamma - 1.0) * ( 1.0 + offs)), gamma);
  const float rs = pow( ( gamma - 1.0) / offs, gamma - 1.0) * pow( ( 1.0 + offs) / gamma, gamma);
  if ( y >= yb) 
    x = ( 1.0 + offs) * pow( y, 1.0 / gamma) - offs;
  else
    x = y * rs;
  return x;
}

float3 moncurve_f_f3( float3 x, float gamma, float offs)
{
    float3 y;
    y[0] = moncurve_f( x[0], gamma, offs);
    y[1] = moncurve_f( x[1], gamma, offs);
    y[2] = moncurve_f( x[2], gamma, offs);
    return y;
}

float3 moncurve_r_f3( float3 y, float gamma, float offs)
{
    float3 x;
    x[0] = moncurve_r( y[0], gamma, offs);
    x[1] = moncurve_r( y[1], gamma, offs);
    x[2] = moncurve_r( y[2], gamma, offs);
    return x;
}

float bt1886_f( float V, float gamma, float Lw, float Lb)
{
  // The reference EOTF specified in Rec. ITU-R BT.1886
  // L = a(max[(V+b),0])^g
  float a = pow( pow( Lw, 1./gamma) - pow( Lb, 1./gamma), gamma);
  float b = pow( Lb, 1./gamma) / ( pow( Lw, 1./gamma) - pow( Lb, 1./gamma));
  float L = a * pow( max( V + b, 0.), gamma);
  return L;
}

float bt1886_r( float L, float gamma, float Lw, float Lb)
{
  // The reference EOTF specified in Rec. ITU-R BT.1886
  // L = a(max[(V+b),0])^g
  float a = pow( pow( Lw, 1./gamma) - pow( Lb, 1./gamma), gamma);
  float b = pow( Lb, 1./gamma) / ( pow( Lw, 1./gamma) - pow( Lb, 1./gamma));
  float V = pow( max( L / a, 0.), 1./gamma) - b;
  return V;
}

float3 bt1886_f_f3( float3 V, float gamma, float Lw, float Lb)
{
    float3 L;
    L[0] = bt1886_f( V[0], gamma, Lw, Lb);
    L[1] = bt1886_f( V[1], gamma, Lw, Lb);
    L[2] = bt1886_f( V[2], gamma, Lw, Lb);
    return L;
}

float3 bt1886_r_f3( float3 L, float gamma, float Lw, float Lb)
{
    float3 V;
    V[0] = bt1886_r( L[0], gamma, Lw, Lb);
    V[1] = bt1886_r( L[1], gamma, Lw, Lb);
    V[2] = bt1886_r( L[2], gamma, Lw, Lb);
    return V;
}

// SMPTE Range vs Full Range scaling formulas
float smpteRange_to_fullRange( float val)
{
    const float REFBLACK = (  64. / 1023.);
    const float REFWHITE = ( 940. / 1023.);

    return (( val - REFBLACK) / ( REFWHITE - REFBLACK));
}

float fullRange_to_smpteRange( float val)
{
    const float REFBLACK = (  64. / 1023.);
    const float REFWHITE = ( 940. / 1023.);

    return ( val * ( REFWHITE - REFBLACK) + REFBLACK );
}

float3 smpteRange_to_fullRange_f3( float3 rgbIn)
{
    float3 rgbOut;
    rgbOut[0] = smpteRange_to_fullRange( rgbIn[0]);
    rgbOut[1] = smpteRange_to_fullRange( rgbIn[1]);
    rgbOut[2] = smpteRange_to_fullRange( rgbIn[2]);

    return rgbOut;
}

float3 fullRange_to_smpteRange_f3( float3 rgbIn)
{
    float3 rgbOut;
    rgbOut[0] = fullRange_to_smpteRange( rgbIn[0]);
    rgbOut[1] = fullRange_to_smpteRange( rgbIn[1]);
    rgbOut[2] = fullRange_to_smpteRange( rgbIn[2]);

    return rgbOut;
}

#endif
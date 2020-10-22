#ifndef __aces_utilities_color__
#define __aces_utilities_color__
// !!!!!!! None of this code has been checked against reference !!!!!!!!
// use at your own risk
// All modifications are purely mine and some errors might have slipped in while hand translating to hlsl
// You have been warned.
/*
https://github.com/ampas/aces-dev

Academy Color Encoding System (ACES) software and tools are provided by the Academy under the following terms and conditions: A worldwide, royalty-free, non-exclusive right to copy, modify, create derivatives, and use, in source and binary forms, is hereby granted, subject to acceptance of this license.

Copyright 2019 Academy of Motion Picture Arts and Sciences (A.M.P.A.S.). Portions contributed by others as indicated. All rights reserved.

Performance of any of the aforementioned acts indicates acceptance to be bound by the following terms and conditions:

Copies of source code, in whole or in part, must retain the above copyright notice, this list of conditions and the Disclaimer of Warranty.

Use in binary form must retain the above copyright notice, this list of conditions and the Disclaimer of Warranty in the documentation and/or other materials provided with the distribution.

Nothing in this license shall be deemed to grant any rights to trademarks, copyrights, patents, trade secrets or any other intellectual property of A.M.P.A.S. or any contributors, except as expressly stated herein.

Neither the name "A.M.P.A.S." nor the name of any other contributors to this software may be used to endorse or promote products derivative of or based on this software without express prior written permission of A.M.P.A.S. or the contributors, as appropriate.

This license shall be construed pursuant to the laws of the State of California, and any disputes related thereto shall be subject to the jurisdiction of the courts therein.

Disclaimer of Warranty: THIS SOFTWARE IS PROVIDED BY A.M.P.A.S. AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT SHALL A.M.P.A.S., OR ANY CONTRIBUTORS OR DISTRIBUTORS, BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, RESITUTIONARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

WITHOUT LIMITING THE GENERALITY OF THE FOREGOING, THE ACADEMY SPECIFICALLY DISCLAIMS ANY REPRESENTATIONS OR WARRANTIES WHATSOEVER RELATED TO PATENT OR OTHER INTELLECTUAL PROPERTY RIGHTS IN THE ACADEMY COLOR ENCODING SYSTEM, OR APPLICATIONS THEREOF, HELD BY PARTIES OTHER THAN A.M.P.A.S.,WHETHER DISCLOSED OR UNDISCLOSED.
*/
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

// SMPTE 431-2 defines the DCDM color encoding equations. 
// The equations for the decoding of the encoded color information are the 
// inverse of the encoding equations
// Note: Here the 4095 12-bit scalar is not used since the output of CTL is 0-1.
float3 dcdm_decode( float3 XYZp)
{
    float3 XYZ;
    XYZ[0] = (52.37/48.0) * pow( XYZp[0], 2.6);  
    XYZ[1] = (52.37/48.0) * pow( XYZp[1], 2.6);  
    XYZ[2] = (52.37/48.0) * pow( XYZp[2], 2.6);  

    return XYZ;
}

float3 dcdm_encode( float3 XYZ)
{
    float3 XYZp;
    XYZp[0] = pow( (48./52.37) * XYZ[0], 1./2.6);
    XYZp[1] = pow( (48./52.37) * XYZ[1], 1./2.6);
    XYZp[2] = pow( (48./52.37) * XYZ[2], 1./2.6);

    return XYZp;
}



// Base functions from SMPTE ST 2084-2014

// Constants from SMPTE ST 2084-2014
static const float pq_m1 = 0.1593017578125; // ( 2610.0 / 4096.0 ) / 4.0;
static const float pq_m2 = 78.84375; // ( 2523.0 / 4096.0 ) * 128.0;
static const float pq_c1 = 0.8359375; // 3424.0 / 4096.0 or pq_c3 - pq_c2 + 1.0;
static const float pq_c2 = 18.8515625; // ( 2413.0 / 4096.0 ) * 32.0;
static const float pq_c3 = 18.6875; // ( 2392.0 / 4096.0 ) * 32.0;

static const float pq_C = 10000.0;

// Converts from the non-linear perceptually quantized space to linear cd/m^2
// Note that this is in float, and assumes normalization from 0 - 1
// (0 - pq_C for linear) and does not handle the integer coding in the Annex 
// sections of SMPTE ST 2084-2014
float ST2084_2_Y( float N )
{
  // Note that this does NOT handle any of the signal range
  // considerations from 2084 - this assumes full range (0 - 1)
  float Np = pow( N, 1.0 / pq_m2 );
  float L = Np - pq_c1;
  if ( L < 0.0 )
    L = 0.0;
  L = L / ( pq_c2 - pq_c3 * Np );
  L = pow( L, 1.0 / pq_m1 );
  return L * pq_C; // returns cd/m^2
}

// Converts from linear cd/m^2 to the non-linear perceptually quantized space
// Note that this is in float, and assumes normalization from 0 - 1
// (0 - pq_C for linear) and does not handle the integer coding in the Annex 
// sections of SMPTE ST 2084-2014
float Y_2_ST2084( float C )
//pq_r
{
  // Note that this does NOT handle any of the signal range
  // considerations from 2084 - this returns full range (0 - 1)
  float L = C / pq_C;
  float Lm = pow( L, pq_m1 );
  float N = ( pq_c1 + pq_c2 * Lm ) / ( 1.0 + pq_c3 * Lm );
  N = pow( N, pq_m2 );
  return N;
}

float3 Y_2_ST2084_f3( float3 input)
{
  // converts from linear cd/m^2 to PQ code values
  
  float3 output;
  output[0] = Y_2_ST2084( input[0]);
  output[1] = Y_2_ST2084( input[1]);
  output[2] = Y_2_ST2084( input[2]);

  return output;
}

float3 ST2084_2_Y_f3( float3 input)
{
  // converts from PQ code values to linear cd/m^2
  
  float3 output;
  output[0] = ST2084_2_Y( input[0]);
  output[1] = ST2084_2_Y( input[1]);
  output[2] = ST2084_2_Y( input[2]);

  return output;
}


// Conversion of PQ signal to HLG, as detailed in Section 7 of ITU-R BT.2390-0
float3 ST2084_2_HLG_1000nits_f3( float3 PQ) 
{
    // ST.2084 EOTF (non-linear PQ to display light)
    float3 displayLinear = ST2084_2_Y_f3( PQ);

    // HLG Inverse EOTF (i.e. HLG inverse OOTF followed by the HLG OETF)
    // HLG Inverse OOTF (display linear to scene linear)
    float Y_d = 0.2627*displayLinear[0] + 0.6780*displayLinear[1] + 0.0593*displayLinear[2];
    const float L_w = 1000.;
    const float L_b = 0.;
    const float alpha = (L_w-L_b);
    const float beta = L_b;
    const float gamma = 1.2;
    
    float3 sceneLinear;
    if (Y_d == 0.) { 
        /* This case is to protect against pow(0,-N)=Inf error. The ITU document
        does not offer a recommendation for this corner case. There may be a 
        better way to handle this, but for now, this works. 
        */ 
        sceneLinear[0] = 0.;
        sceneLinear[1] = 0.;
        sceneLinear[2] = 0.;        
    } else {
        sceneLinear[0] = pow( (Y_d-beta)/alpha, (1.-gamma)/gamma) * ((displayLinear[0]-beta)/alpha);
        sceneLinear[1] = pow( (Y_d-beta)/alpha, (1.-gamma)/gamma) * ((displayLinear[1]-beta)/alpha);
        sceneLinear[2] = pow( (Y_d-beta)/alpha, (1.-gamma)/gamma) * ((displayLinear[2]-beta)/alpha);
    }

    // HLG OETF (scene linear to non-linear signal value)
    const float a = 0.17883277;
    const float b = 0.28466892; // 1.-4.*a;
    const float c = 0.55991073; // 0.5-a*log(4.*a);

    float3 HLG;
    if (sceneLinear[0] <= 1./12) {
        HLG[0] = sqrt(3.*sceneLinear[0]);
    } else {
        HLG[0] = a*log(12.*sceneLinear[0]-b)+c;
    }
    if (sceneLinear[1] <= 1./12) {
        HLG[1] = sqrt(3.*sceneLinear[1]);
    } else {
        HLG[1] = a*log(12.*sceneLinear[1]-b)+c;
    }
    if (sceneLinear[2] <= 1./12) {
        HLG[2] = sqrt(3.*sceneLinear[2]);
    } else {
        HLG[2] = a*log(12.*sceneLinear[2]-b)+c;
    }

    return HLG;
}


// Conversion of HLG to PQ signal, as detailed in Section 7 of ITU-R BT.2390-0
float3 HLG_2_ST2084_1000nits_f3( float3 HLG) 
{
    const float a = 0.17883277;
    const float b = 0.28466892; // 1.-4.*a;
    const float c = 0.55991073; // 0.5-a*log(4.*a);

    const float L_w = 1000.;
    const float L_b = 0.;
    const float alpha = (L_w-L_b);
    const float beta = L_b;
    const float gamma = 1.2;

    // HLG EOTF (non-linear signal value to display linear)
    // HLG to scene-linear
    float sceneLinear[3];
    if ( HLG[0] >= 0. && HLG[0] <= 0.5) {
        sceneLinear[0] = pow(HLG[0],2.)/3.;
    } else {
        sceneLinear[0] = (exp((HLG[0]-c)/a)+b)/12.;
    }        
    if ( HLG[1] >= 0. && HLG[1] <= 0.5) {
        sceneLinear[1] = pow(HLG[1],2.)/3.;
    } else {
        sceneLinear[1] = (exp((HLG[1]-c)/a)+b)/12.;
    }        
    if ( HLG[2] >= 0. && HLG[2] <= 0.5) {
        sceneLinear[2] = pow(HLG[2],2.)/3.;
    } else {
        sceneLinear[2] = (exp((HLG[2]-c)/a)+b)/12.;
    }        
    
    float Y_s = 0.2627*sceneLinear[0] + 0.6780*sceneLinear[1] + 0.0593*sceneLinear[2];

    // Scene-linear to display-linear
    float3 displayLinear;
    displayLinear[0] = alpha * pow( Y_s, gamma-1.) * sceneLinear[0] + beta;
    displayLinear[1] = alpha * pow( Y_s, gamma-1.) * sceneLinear[1] + beta;
    displayLinear[2] = alpha * pow( Y_s, gamma-1.) * sceneLinear[2] + beta;
        
    // ST.2084 Inverse EOTF
    float3 PQ = Y_2_ST2084_f3( displayLinear);

    return PQ;
}

#endif
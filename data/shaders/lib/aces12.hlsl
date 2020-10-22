#ifndef __aces12__
#define __aces12__

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

#include "aces_constants.hlsl"
#include "aces_transforms.hlsl"
// ACES Color Space Conversion - ACES to ACEScg
//
// converts ACES2065-1 (AP0 w/ linear encoding) to 
//          ACEScg (AP1 w/ linear encoding)
float4 ACESToACEScg(float4 ACES) {
  ACES.rgb = clamp( ACES.rgb, 0.0, HALF_POS_INF);
  return float4(mult_f3_f44(ACES.rgb, AP0_2_AP1_MAT), ACES.a);
}

float4 ACEScgToACES(float4 ACEScg) {
  return float4(mult_f3_f44(ACEScg.rgb, AP1_2_AP0_MAT), ACEScg.a);
}

float lin_to_ACEScc(float value) {
  if (value <= 0)
    return -0.3584474886f; // =(log2( pow(2.,-16.))+9.72)/17.52
  else if (value < pow(2.f,-15.f))
    return (log2( pow(2.f,-16.f) + value * 0.5f) + 9.72f) / 17.52f;
  else // (value >= pow(2.,-15))
    return (log2(value + 9.72f) / 17.52f);
}

float4 ACESToACEScc(float4 ACES) {
    ACES.rgb = clamp( ACES.rgb, 0.0f, HALF_POS_INF);
    // NOTE: (from Annex A of S-2014-003)
    // When ACES values are matrixed into the smaller AP1 space, colors outside 
    // the AP1 gamut can generate negative values even before the log encoding. 
    // If these values are clipped, a conversion back to ACES will not restore 
    // the original colors. A specific method of reserving negative values 
    // produced by the transformation matrix has not been defined in part to 
    // help ease adoption across various color grading systems that have 
    // different capabilities and methods for handling negative values. Clipping 
    // these values has been found to have minimal visual impact when viewed 
    // through the RRT and ODT on currently available display technology. 
    // However, to preserve creative choice in downstream processing and to 
    // provide the highest quality archival master, developers implementing 
    // ACEScc encoding are encouraged to adopt a method of preserving negative 
    // values so that a conversion from ACES to ACEScc and back can be made 
    // lossless.

    float3 lin_AP1 = mult_f3_f44(ACES.rgb, AP0_2_AP1_MAT);
    //lin_AP1 = ACES.rgb;
    return float4(lin_to_ACEScc( lin_AP1.r),
                  lin_to_ACEScc( lin_AP1.g),
                  lin_to_ACEScc( lin_AP1.b), ACES.a);
}

float ACEScc_to_lin(float value) {
  if (value < -0.3013698630f) // (9.72-15)/17.52
    return (pow( 2.f, value*17.52f-9.72f) - pow( 2.f,-16.f))*2.0f;
  else if ( value < (log2(HALF_MAX)+9.72f)/17.52f )
    return pow( 2.f, value*17.52f-9.72f);
  else // (value >= (log2(HALF_MAX)+9.72)/17.52)
    return HALF_MAX;
}

float4 ACESccToACES(float4 ACEScc)
{
  float3 lin_AP1;
  lin_AP1.r = ACEScc_to_lin(ACEScc.r);
  lin_AP1.g = ACEScc_to_lin(ACEScc.g);
  lin_AP1.b = ACEScc_to_lin(ACEScc.b);
  float3 temp = mult_f3_f44(lin_AP1, AP1_2_AP0_MAT);
  //temp = lin_AP1;

  return float4(temp, ACEScc.a);
}

static const float3x3 rgbToAcesCG = float3x3(0.6131178129,0.3411819959,0.0457873443,
                                              0.0699340823,0.9181030375,0.0119327755,
                                              0.0204629926,0.1067686634,0.8727159106); 
float4 sRGBToAcesCg(float4 input) {
#if 1
  float3 xyz = mult_f3_f44(input.rgb, RGBtoXYZ(REC709_PRI,1.0));
  xyz = mult_f3_f44(xyz, XYZ_2_AP1_MAT);
  return float4(xyz, input.a);
#else
  float3 xyz = mul(input.rgb, rgbToAcesCG);
  return float4(xyz, input.a);
#endif
}
//
// Contains functions and constants shared by forward and inverse RRT transforms
//

// "Glow" module constants
static const float RRT_GLOW_GAIN = 0.05f;
static const float RRT_GLOW_MID = 0.08f;

// Red modifier constants
static const float RRT_RED_SCALE = 0.82f;
static const float RRT_RED_PIVOT = 0.03f;
static const float RRT_RED_HUE = 0.f;
static const float RRT_RED_WIDTH = 135.f;

// Desaturation contants
static const float RRT_SAT_FACTOR = 0.96f;
static const float3x3 RRT_SAT_MAT = calc_sat_adjust_matrix( RRT_SAT_FACTOR, AP1_RGB2Y);




// ------- Glow module functions
float glow_fwd( float ycIn, float glowGainIn, float glowMid)
{
   float glowGainOut;

   if (ycIn <= 2./3. * glowMid) {
     glowGainOut = glowGainIn;
   } else if ( ycIn >= 2. * glowMid) {
     glowGainOut = 0.;
   } else {
     glowGainOut = glowGainIn * (glowMid / ycIn - 1./2.);
   }

   return glowGainOut;
}

float glow_inv( float ycOut, float glowGainIn, float glowMid)
{
    float glowGainOut;

    if (ycOut <= ((1 + glowGainIn) * 2./3. * glowMid)) {
    glowGainOut = -glowGainIn / (1 + glowGainIn);
    } else if ( ycOut >= (2. * glowMid)) {
    glowGainOut = 0.;
    } else {
    glowGainOut = glowGainIn * (glowMid / ycOut - 1./2.) / (glowGainIn / 2. - 1.);
    }

    return glowGainOut;
}

float sigmoid_shaper( float x)
{
    // Sigmoid function in the range 0 to 1 spanning -2 to +2.

    float t = max( 1. - abs( x / 2.), 0.);
    float y = 1. + sign(x) * (1. - t * t);

    return y / 2.;
}


// ------- Red modifier functions
float cubic_basis_shaper(float x, float w ) {  // full base width of the shaper function (in degrees)
  float4x4 M = float4x4( -1.f/6.f,  3.f/6.f, -3.f/6.f,  1.f/6.f,
                     3.f/6.f, -6.f/6.f,  3.f/6.f,  0.f/6.f,
                    -3.f/6.f,  0.f/6.f,  3.f/6.f,  0.f/6.f,
                     1.f/6.f,  4.f/6.f,  1.f/6.f,  0.f/6.f);
  
  float knots[5] = { -w/2.,
                     -w/4.,
                     0.,
                     w/4.,
                     w/2. };
  
  float y = 0;
  if ((x > knots[0]) && (x < knots[4])) {  
    float knot_coord = (x - knots[0]) * 4./w;  
    int j = knot_coord;
    float t = knot_coord - j;
      
    float monomials[4] = { t*t*t, t*t, t, 1. };

    // (if/else structure required for compatibility with CTL < v1.5.)
    if ( j == 3) {
      y = monomials[0] * M[0][0] + monomials[1] * M[1][0] + 
          monomials[2] * M[2][0] + monomials[3] * M[3][0];
    } else if ( j == 2) {
      y = monomials[0] * M[0][1] + monomials[1] * M[1][1] + 
          monomials[2] * M[2][1] + monomials[3] * M[3][1];
    } else if ( j == 1) {
      y = monomials[0] * M[0][2] + monomials[1] * M[1][2] + 
          monomials[2] * M[2][2] + monomials[3] * M[3][2];
    } else if ( j == 0) {
      y = monomials[0] * M[0][3] + monomials[1] * M[1][3] + 
          monomials[2] * M[2][3] + monomials[3] * M[3][3];
    } else {
      y = 0.0f;
    }
  }
  
  return y * 3.f/2.f;
}

float center_hue( float hue, float centerH) {
  float hueCentered = hue - centerH;
  if (hueCentered < -180.f) hueCentered = hueCentered + 360.f;
  else if (hueCentered > 180.f) hueCentered = hueCentered - 360.f;
  return hueCentered;
}

float uncenter_hue( float hueCentered, float centerH) {
  float hue = hueCentered + centerH;
  if (hue < 0.) hue = hue + 360.;
  else if (hue > 360.) hue = hue - 360.;
  return hue;
}

float3 rrt_sweeteners( float3 val)
{
    float3 aces = val;
    
    // --- Glow module --- //
    float saturation = rgb_2_saturation( aces);
    float ycIn = rgb_2_yc(aces);
    float s = sigmoid_shaper( (saturation - 0.4) / 0.2);
    float addedGlow = 1. + glow_fwd( ycIn, RRT_GLOW_GAIN * s, RRT_GLOW_MID);

    aces = mul( addedGlow, aces);

    // --- Red modifier --- //
    float hue = rgb_2_hue( aces);
    float centeredHue = center_hue( hue, RRT_RED_HUE);
    float hueWeight = cubic_basis_shaper( centeredHue, RRT_RED_WIDTH);

    aces[0] = aces[0] + hueWeight * saturation * (RRT_RED_PIVOT - aces[0]) * (1. - RRT_RED_SCALE);

    // --- ACES to RGB rendering space --- //
    aces = clamp( aces, 0., HALF_POS_INF);
    float3 rgbPre = mult_f3_f44( aces, AP0_2_AP1_MAT);
    rgbPre = clamp( rgbPre, 0., HALF_MAX);
    
    // --- Global desaturation --- //
    rgbPre = mult_f3_f33( rgbPre, RRT_SAT_MAT);
    return rgbPre;
}

float3 inv_rrt_sweeteners( float3 val)
{
    float3 rgbPost = val;
    
    // --- Global desaturation --- //
    rgbPost = mult_f3_f33( rgbPost, inverse(RRT_SAT_MAT));

    rgbPost = clamp( rgbPost, 0., HALF_MAX);

    // --- RGB rendering space to ACES --- //
    float3 aces = mult_f3_f44( rgbPost, AP1_2_AP0_MAT);

    aces = clamp( aces, 0.f, HALF_MAX);

    // --- Red modifier --- //
    float hue = rgb_2_hue( aces);
    float centeredHue = center_hue( hue, RRT_RED_HUE);
    float hueWeight = cubic_basis_shaper( centeredHue, RRT_RED_WIDTH);

    float minChan;
    if (centeredHue < 0) { // min_f3(aces) = aces[1] (i.e. magenta-red)
      minChan = aces[1];
    } else { // min_f3(aces) = aces[2] (i.e. yellow-red)
      minChan = aces[2];
    }

    float a = hueWeight * (1. - RRT_RED_SCALE) - 1.;
    float b = aces[0] - hueWeight * (RRT_RED_PIVOT + minChan) * (1. - RRT_RED_SCALE);
    float c = hueWeight * RRT_RED_PIVOT * minChan * (1. - RRT_RED_SCALE);

    aces[0] = ( -b - sqrt( b * b - 4. * a * c)) / ( 2. * a);

    // --- Glow module --- //
    float saturation = rgb_2_saturation( aces);
    float ycOut = rgb_2_yc(aces);
    float s = sigmoid_shaper( (saturation - 0.4) / 0.2);
    float reducedGlow = 1. + glow_inv( ycOut, RRT_GLOW_GAIN * s, RRT_GLOW_MID);

    aces = mul( ( reducedGlow), aces);
    return aces;
}

// 
// Reference Rendering Transform (RRT)
//
//   Input is ACES
//   Output is OCES
//
float4 ACESrrt(float4 ACES) {
  // --- Initialize a 3-element vector with input variables (ACES) --- //
  float3 aces = ACES.rgb;

  // --- Glow module --- //
    float saturation = rgb_2_saturation( aces);
    float ycIn = rgb_2_yc( aces);
    float s = sigmoid_shaper( (saturation - 0.4) / 0.2);
    float addedGlow = 1. + glow_fwd( ycIn, RRT_GLOW_GAIN * s, RRT_GLOW_MID);

    aces = mul( addedGlow, aces);

  // --- Red modifier --- //
    float hue = rgb_2_hue( aces);
    float centeredHue = center_hue( hue, RRT_RED_HUE);
    float hueWeight = cubic_basis_shaper( centeredHue, RRT_RED_WIDTH);

    aces[0] = aces[0] + hueWeight * saturation * (RRT_RED_PIVOT - aces[0]) * (1. - RRT_RED_SCALE);

  // --- ACES to RGB rendering space --- //
    aces = clamp( aces, 0., HALF_POS_INF);  // avoids saturated negative colors from becoming positive in the matrix

    float3 rgbPre = mult_f3_f44(aces, AP0_2_AP1_MAT);

    rgbPre = clamp( rgbPre, 0., HALF_MAX);

  // --- Global desaturation --- //
    rgbPre = mult_f3_f33( rgbPre, RRT_SAT_MAT);

  // --- Apply the tonescale independently in rendering-space RGB --- //
    float3 rgbPost;
    rgbPost[0] = segmented_spline_c5_fwd( rgbPre[0]);
    rgbPost[1] = segmented_spline_c5_fwd( rgbPre[1]);
    rgbPost[2] = segmented_spline_c5_fwd( rgbPre[2]);

  // --- RGB rendering space to OCES --- //
    float3 rgbOces = mult_f3_f44(rgbPost, AP1_2_AP0_MAT);

  // Assign OCES RGB to output variables (OCES)
  return float4(rgbOces, ACES.w);
}

// 
// Inverse Reference Rendering Transform (RRT)
//
//   Input is OCES
//   Output is ACES
//

float4 inverseACESrrt(float4 OCES)
{
  // --- Initialize a 3-element vector with input variables (OCES) --- //
    float3 oces = OCES.rgb;

  // --- OCES to RGB rendering space --- //
    float3 rgbPre = mult_f3_f44(oces, AP0_2_AP1_MAT);

  // --- Apply the tonescale independently in rendering-space RGB --- //
    float3 rgbPost;
    rgbPost[0] = segmented_spline_c5_rev( rgbPre[0]);
    rgbPost[1] = segmented_spline_c5_rev( rgbPre[1]);
    rgbPost[2] = segmented_spline_c5_rev( rgbPre[2]);

  // --- Global desaturation --- //
    rgbPost = mult_f3_f33( rgbPost, inverse(RRT_SAT_MAT));

    rgbPost = clamp( rgbPost, 0., HALF_MAX);

  // --- RGB rendering space to ACES --- //
    float3 aces = mult_f3_f44(rgbPost, AP1_2_AP0_MAT);

    aces = clamp( aces, 0.f, HALF_MAX);

  // --- Red modifier --- //
    float hue = rgb_2_hue( aces);
    float centeredHue = center_hue( hue, RRT_RED_HUE);
    float hueWeight = cubic_basis_shaper( centeredHue, RRT_RED_WIDTH);

    float minChan;
    if (centeredHue < 0) { // min_f3(aces) = aces[1] (i.e. magenta-red)
      minChan = aces[1];
    } else { // min_f3(aces) = aces[2] (i.e. yellow-red)
      minChan = aces[2];
    }

    float a = hueWeight * (1. - RRT_RED_SCALE) - 1.;
    float b = aces[0] - hueWeight * (RRT_RED_PIVOT + minChan) * (1. - RRT_RED_SCALE);
    float c = hueWeight * RRT_RED_PIVOT * minChan * (1. - RRT_RED_SCALE);

    aces[0] = ( -b - sqrt( b * b - 4. * a * c)) / ( 2. * a);

  // --- Glow module --- //
    float saturation = rgb_2_saturation( aces);
    float ycOut = rgb_2_yc( aces);
    float s = sigmoid_shaper( (saturation - 0.4) / 0.2);
    float reducedGlow = 1. + glow_inv( ycOut, RRT_GLOW_GAIN * s, RRT_GLOW_MID);

    aces = mul( ( reducedGlow), aces);

  // Assign ACES RGB to output variables (ACES)
  return float4(aces, OCES.w);
}

#endif
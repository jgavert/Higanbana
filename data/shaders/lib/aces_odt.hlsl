#ifndef __aces_odt__
#define __aces_odt__

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
#include "aces_utilities_color.hlsl"

//
// ODT common.ctl
//
// Target white and black points for cinema system tonescale
static const float CINEMA_WHITE = 48.0f;
static const float CINEMA_BLACK = pow10(log10(0.02f)); // CINEMA_WHITE / 2400. 
    // CINEMA_BLACK is defined in this roundabout manner in order to be exactly equal to 
    // the result returned by the cinema 48-nit ODT tonescale.
    // Though the min point of the tonescale is designed to return 0.02, the tonescale is 
    // applied in log-log space, which loses precision on the antilog. The tonescale 
    // return value is passed into Y_2_linCV, where CINEMA_BLACK is subtracted. If 
    // CINEMA_BLACK is defined as simply 0.02, then the return value of this subfunction
    // is very, very small but not equal to 0, and attaining a CV of 0 is then impossible.
    // For all intents and purposes, CINEMA_BLACK=0.02.
// Gamma compensation factor
static const float DIM_SURROUND_GAMMA = 0.9811f;

// Saturation compensation factor
static const float ODT_SAT_FACTOR = 0.93;
static const float3x3 ODT_SAT_MAT = calc_sat_adjust_matrix( ODT_SAT_FACTOR, AP1_RGB2Y);

static const float3x3 D60_2_D65_CAT = calculate_cat_matrix( AP0.white, REC709_PRI.white);

float Y_2_linCV( float Y, float Ymax, float Ymin) {
  return (Y - Ymin) / (Ymax - Ymin);
}

float linCV_2_Y( float linCV, float Ymax, float Ymin) {
  return linCV * (Ymax - Ymin) + Ymin;
}

float3 Y_2_linCV_f3( float3 Y, float Ymax, float Ymin) {
    float3 linCV;
    linCV[0] = Y_2_linCV( Y[0], Ymax, Ymin);
    linCV[1] = Y_2_linCV( Y[1], Ymax, Ymin);
    linCV[2] = Y_2_linCV( Y[2], Ymax, Ymin);
    return linCV;
}

float3 linCV_2_Y_f3( float3 linCV, float Ymax, float Ymin) {
    float3 Y;
    Y[0] = linCV_2_Y( linCV[0], Ymax, Ymin);
    Y[1] = linCV_2_Y( linCV[1], Ymax, Ymin);
    Y[2] = linCV_2_Y( linCV[2], Ymax, Ymin);
    return Y;
}

float3 darkSurround_to_dimSurround( float3 linearCV) {
  float3 XYZ = mult_f3_f44(linearCV, AP1_2_XYZ_MAT); 

  float3 xyY = XYZ_2_xyY(XYZ);
  xyY[2] = clamp( xyY[2], 0., HALF_POS_INF);
  xyY[2] = pow( xyY[2], DIM_SURROUND_GAMMA);
  XYZ = xyY_2_XYZ(xyY);

  return mult_f3_f44(XYZ, XYZ_2_AP1_MAT);
}

float3 dimSurround_to_darkSurround( float3 linearCV)
{
  float3 XYZ = mult_f3_f44(linearCV, AP1_2_XYZ_MAT); 

  float3 xyY = XYZ_2_xyY(XYZ);
  xyY[2] = clamp( xyY[2], 0., HALF_POS_INF);
  xyY[2] = pow( xyY[2], 1./DIM_SURROUND_GAMMA);
  XYZ = xyY_2_XYZ(xyY);

  return mult_f3_f44(XYZ, XYZ_2_AP1_MAT);
}

float roll_white_fwd( 
    float input,      // color value to adjust (white scaled to around 1.0)
    float new_wht, // white adjustment (e.g. 0.9 for 10% darkening)
    float width    // adjusted width (e.g. 0.25 for top quarter of the tone scale)
  )
{
    const float x0 = -1.0;
    const float x1 = x0 + width;
    const float y0 = -new_wht;
    const float y1 = x1;
    const float m1 = (x1 - x0);
    const float a = y0 - y1 + m1;
    const float b = 2 * ( y1 - y0) - m1;
    const float c = y0;
    const float t = (-input - x0) / (x1 - x0);
    float output = 0.0;
    if ( t < 0.0)
        output = -(t * b + c);
    else if ( t > 1.0)
        output = input;
    else
        output = -(( t * a + b) * t + c);
    return output;
}

float roll_white_rev( 
    float input,      // color value to adjust (white scaled to around 1.0)
    float new_wht, // white adjustment (e.g. 0.9 for 10% darkening)
    float width    // adjusted width (e.g. 0.25 for top quarter of the tone scale)
  )
{
    const float x0 = -1.0;
    const float x1 = x0 + width;
    const float y0 = -new_wht;
    const float y1 = x1;
    const float m1 = (x1 - x0);
    const float a = y0 - y1 + m1;
    const float b = 2. * ( y1 - y0) - m1;
    float c = y0;
    float output = 0.0;
    if ( -input < y0)
        output = -x0;
    else if ( -input > y1)
        output = input;
    else {
        c = c + input;
        const float discrim = sqrt( b * b - 4. * a * c);
        const float t = ( 2. * c) / ( -discrim - b);
        output = -(( t * ( x1 - x0)) + x0);
    }
    return output;
}

// 
// Inverse Output Device Transform - RGB computer monitor
//

/* ----- ODT Parameters ------ */
//static const Chromaticities DISPLAY_PRI = REC709_PRI;
//static const float4x4 DISPLAY_PRI_2_XYZ_MAT = RGBtoXYZ(DISPLAY_PRI,1.0);


float3 inverseODTRGB(float3 rgb) {  
    float3 outputCV = rgb;
    static const float DISPGAMMA = 2.2; 
    static const float OFFSET = 0.055;

    static const float4x4 DISPLAY_PRI_2_XYZ_MAT = RGBtoXYZ(REC709_PRI,1.0);

    // Decode to linear code values with inverse transfer function
    float3 linearCV;
    // moncurve_f with gamma of 2.4 and offset of 0.055 matches the EOTF found in IEC 61966-2-1:1999 (sRGB)
    linearCV[0] = moncurve_f( outputCV[0], DISPGAMMA, OFFSET);
    linearCV[1] = moncurve_f( outputCV[1], DISPGAMMA, OFFSET);
    linearCV[2] = moncurve_f( outputCV[2], DISPGAMMA, OFFSET);

    // Convert from display primary encoding
    // Display primaries to CIE XYZ
    float3 XYZ = mult_f3_f44(linearCV, DISPLAY_PRI_2_XYZ_MAT);

    // Apply CAT from assumed observer adapted white to ACES white point
    XYZ = mult_f3_f33( XYZ, inverse( D60_2_D65_CAT));

    // CIE XYZ to rendering space RGB
    linearCV = mult_f3_f44(XYZ, XYZ_2_AP1_MAT);

    // Undo desaturation to compensate for luminance difference
    linearCV = mult_f3_f33( linearCV, inverse( ODT_SAT_MAT));

    // Undo gamma adjustment to compensate for dim surround
    linearCV = dimSurround_to_darkSurround( linearCV);

    // Scale linear code value to luminance
    float3 rgbPre;
    rgbPre[0] = linCV_2_Y( linearCV[0], CINEMA_WHITE, CINEMA_BLACK);
    rgbPre[1] = linCV_2_Y( linearCV[1], CINEMA_WHITE, CINEMA_BLACK);
    rgbPre[2] = linCV_2_Y( linearCV[2], CINEMA_WHITE, CINEMA_BLACK);

    // Apply the tonescale independently in rendering-space RGB
    float3 rgbPost;
    rgbPost[0] = segmented_spline_c9_rev( rgbPre[0]);
    rgbPost[1] = segmented_spline_c9_rev( rgbPre[1]);
    rgbPost[2] = segmented_spline_c9_rev( rgbPre[2]);

    // Rendering space RGB to OCES
    float3 oces = mult_f3_f44(rgbPost, AP1_2_AP0_MAT);
    return oces;
}

// 
// Output Device Transform - RGB computer monitor
//

//
// Summary :
//  This transform is intended for mapping OCES onto a desktop computer monitor 
//  typical of those used in motion picture visual effects production. These 
//  monitors may occasionally be referred to as "sRGB" displays, however, the 
//  monitor for which this transform is designed does not exactly match the 
//  specifications in IEC 61966-2-1:1999.
// 
//  The assumed observer adapted white is D65, and the viewing environment is 
//  that of a dim surround. 
//
//  The monitor specified is intended to be more typical of those found in 
//  visual effects production.
//
// Device Primaries : 
//  Primaries are those specified in Rec. ITU-R BT.709
//  CIE 1931 chromaticities:  x         y         Y
//              Red:          0.64      0.33
//              Green:        0.3       0.6
//              Blue:         0.15      0.06
//              White:        0.3127    0.329     100 cd/m^2
//
// Display EOTF :
//  The reference electro-optical transfer function specified in 
//  IEC 61966-2-1:1999.
//  Note: This EOTF is *NOT* gamma 2.2
//
// Signal Range:
//    This transform outputs full range code values.
//
// Assumed observer adapted white point:
//         CIE 1931 chromaticities:    x            y
//                                     0.3127       0.329
//
// Viewing Environment:
//   This ODT has a compensation for viewing environment variables more typical 
//   of those associated with video mastering.
//

float4 odtRGB(float4 OCES) {
    /* --- ODT Parameters --- */
    static const float4x4 XYZ_2_DISPLAY_PRI_MAT = XYZtoRGB(REC709_PRI,1.0);

    // NOTE: The EOTF is *NOT* gamma 2.4, it follows IEC 61966-2-1:1999
    static const float DISPGAMMA = 2.2; 
    static const float OFFSET = 0.055;

    float3 oces = OCES.rgb;

    // OCES to RGB rendering space
    float3 rgbPre = mult_f3_f44(oces, AP0_2_AP1_MAT);

    // Apply the tonescale independently in rendering-space RGB
    float3 rgbPost;
    rgbPost[0] = segmented_spline_c9_fwd( rgbPre[0]);
    rgbPost[1] = segmented_spline_c9_fwd( rgbPre[1]);
    rgbPost[2] = segmented_spline_c9_fwd( rgbPre[2]);

    // Scale luminance to linear code value
    float3 linearCV;
    linearCV[0] = Y_2_linCV( rgbPost[0], CINEMA_WHITE, CINEMA_BLACK);
    linearCV[1] = Y_2_linCV( rgbPost[1], CINEMA_WHITE, CINEMA_BLACK);
    linearCV[2] = Y_2_linCV( rgbPost[2], CINEMA_WHITE, CINEMA_BLACK);    

    // Apply gamma adjustment to compensate for dim surround
    linearCV = darkSurround_to_dimSurround( linearCV);

    // Apply desaturation to compensate for luminance difference
    linearCV = mult_f3_f33( linearCV, ODT_SAT_MAT);

    // Convert to display primary encoding
    // Rendering space RGB to XYZ
    float3 XYZ = mult_f3_f44(linearCV, AP1_2_XYZ_MAT);

    // Apply CAT from ACES white point to assumed observer adapted white point
    XYZ = mult_f3_f33( XYZ, D60_2_D65_CAT);

    // CIE XYZ to display primaries
    linearCV = mult_f3_f44( XYZ, XYZ_2_DISPLAY_PRI_MAT);

    // Handle out-of-gamut values
    // Clip values < 0 or > 1 (i.e. projecting outside the display primaries)
    linearCV = clamp( linearCV, 0., 1.);

    // Encode linear code values with transfer function
    float3 outputCV;
    // moncurve_r with gamma of 2.4 and offset of 0.055 matches the EOTF found in IEC 61966-2-1:1999 (sRGB)
    outputCV[0] = moncurve_r( linearCV[0], DISPGAMMA, OFFSET);
    outputCV[1] = moncurve_r( linearCV[1], DISPGAMMA, OFFSET);
    outputCV[2] = moncurve_r( linearCV[2], DISPGAMMA, OFFSET);

    return float4(outputCV, OCES.w);
}

#endif
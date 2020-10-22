#ifndef __aces_output_transforms__
#define __aces_output_transforms__

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

#include "aces_utilities_color.hlsl" // transform common
#include "aces12.hlsl" // transform common
#include "aces_odt.hlsl" // transform common
#include "aces_lib_ssts.hlsl"

//import "ACESlib.Transform_Common";
//import "ACESlib.RRT_Common";
//import "ACESlib.ODT_Common";
//import "ACESlib.SSTS";

float3 limit_to_primaries(float3 XYZ, Chromaticities LIMITING_PRI)
{
    float4x4 XYZ_2_LIMITING_PRI_MAT = XYZtoRGB( LIMITING_PRI, 1.0);
    float4x4 LIMITING_PRI_2_XYZ_MAT = RGBtoXYZ( LIMITING_PRI, 1.0);

    // XYZ to limiting primaries
    float3 rgb = mult_f3_f44( XYZ, XYZ_2_LIMITING_PRI_MAT);

    // Clip any values outside the limiting primaries
    float3 limitedRgb = clamp( rgb, 0., 1.);
    
    // Convert limited RGB to XYZ
    return mult_f3_f44( limitedRgb, LIMITING_PRI_2_XYZ_MAT);
}

float3 dark_to_dim( float3 XYZ)
{
  float3 xyY = XYZ_2_xyY(XYZ);
  xyY.z = clamp( xyY.z, 0., HALF_POS_INF);
  xyY.z = pow( xyY.z, DIM_SURROUND_GAMMA);
  return xyY_2_XYZ(xyY);
}

float3 dim_to_dark( float3 XYZ)
{
  float3 xyY = XYZ_2_xyY(XYZ);
  xyY.z = clamp( xyY.z, 0., HALF_POS_INF);
  xyY.z = pow( xyY.z, 1./DIM_SURROUND_GAMMA);
  return xyY_2_XYZ(xyY);
}

float3 outputTransform(
    float3 input,
    float Y_MIN,
    float Y_MID,
    float Y_MAX,    
    Chromaticities DISPLAY_PRI,
    Chromaticities LIMITING_PRI,
    int EOTF,  
    int SURROUND,
    bool STRETCH_BLACK = true,
    bool D60_SIM = false,
    bool LEGAL_RANGE = false
)
{
    float4x4 XYZ_2_DISPLAY_PRI_MAT = XYZtoRGB( DISPLAY_PRI, 1.0);

    /* 
        NOTE: This is a bit of a hack - probably a more direct way to do this.
        TODO: Fix in future version
    */
    TsParams PARAMS_DEFAULT = init_TsParams( Y_MIN, Y_MAX);
    float expShift = log2(inv_ssts(Y_MID, PARAMS_DEFAULT))-log2(0.18);
    TsParams PARAMS = init_TsParams( Y_MIN, Y_MAX, expShift);


    // RRT sweeteners
    float3 rgbPre = rrt_sweeteners( input);

    // Apply the tonescale independently in rendering-space RGB
    float3 rgbPost = ssts_f3( rgbPre, PARAMS);

    // At this point data encoded AP1, scaled absolute luminance (cd/m^2)

    /*  Scale absolute luminance to linear code value  */
    float3 linearCV = Y_2_linCV_f3( rgbPost, Y_MAX, Y_MIN);
    
    // Rendering primaries to XYZ
    float3 XYZ = mult_f3_f44( linearCV, AP1_2_XYZ_MAT);

    // Apply gamma adjustment to compensate for dim surround
    /*  
        NOTE: This is more or less a placeholder block and is largely inactive 
        in its current form. This section currently only applies for SDR, and
        even then, only in very specific cases.
        In the future it is fully intended for this module to be updated to 
        support surround compensation regardless of luminance dynamic range. */
    /*  
        TOD0: Come up with new surround compensation algorithm, applicable 
        across all dynamic ranges and supporting dark/dim/normal surround.  
    */
    if (SURROUND == 0) { // Dark surround
        /*  
        Current tone scale is designed for dark surround environment so no 
        adjustment is necessary. 
        */
    } else if (SURROUND == 1) { // Dim surround
        // INACTIVE for HDR and crudely implemented for SDR (see comment below)        
        if ((EOTF == 1) || (EOTF == 2) || (EOTF == 3)) { 
            /* 
            This uses a crude logical assumption that if the EOTF is BT.1886,
            sRGB, or gamma 2.6 that the data is SDR and so the SDR gamma
            compensation factor from v1.0 will apply. 
            */
            XYZ = dark_to_dim( XYZ); /*
            This uses a local dark_to_dim function that is designed to take in
            XYZ and return XYZ rather than AP1 as is currently in the functions
            in 'ACESlib.ODT_Common.ctl' */
        }
    } else if (SURROUND == 2) { // Normal surround
        // INACTIVE - this does NOTHING
    }

    // Gamut limit to limiting primaries
    // NOTE: Would be nice to just say
    //    if (LIMITING_PRI != DISPLAY_PRI)
    // but you can't because Chromaticities do not work with bool comparison operator
    // For now, limit no matter what.
    XYZ = limit_to_primaries( XYZ, LIMITING_PRI); 
    
    // Apply CAT from ACES white point to assumed observer adapted white point
    // TODO: Needs to expand from just supporting D60 sim to allow for any
    // observer adapted white point.
    if (D60_SIM == false) {
        if ((DISPLAY_PRI.white[0] != AP0.white[0]) &
            (DISPLAY_PRI.white[1] != AP0.white[1])) {
            float3x3 CAT = calculate_cat_matrix( AP0.white, DISPLAY_PRI.white);
            XYZ = mult_f3_f33( XYZ, D60_2_D65_CAT);
        }
    }

    // CIE XYZ to display encoding primaries
    linearCV = mult_f3_f44( XYZ, XYZ_2_DISPLAY_PRI_MAT);

    // Scale to avoid clipping when device calibration is different from D60. 
    // To simulate D60, unequal code values are sent to the display.
    // TODO: Needs to expand from just supporting D60 sim to allow for any
    // observer adapted white point.
    if (D60_SIM == true) {
        /* TODO: The scale requires calling itself. Scale is same no matter the luminance.
           Currently precalculated for D65, DCI. If DCI, roll_white_fwd is used also.
           This needs a more complex algorithm to handle all cases.
        */
        float SCALE = 1.0;
        if ((DISPLAY_PRI.white[0] == 0.3127) & 
            (DISPLAY_PRI.white[1] == 0.329)) { // D65
                SCALE = 0.96362;
        } 
        else if ((DISPLAY_PRI.white[0] == 0.314) & 
                 (DISPLAY_PRI.white[1] == 0.351)) { // DCI
                linearCV[0] = roll_white_fwd( linearCV[0], 0.918, 0.5);
                linearCV[1] = roll_white_fwd( linearCV[1], 0.918, 0.5);
                linearCV[2] = roll_white_fwd( linearCV[2], 0.918, 0.5);
                SCALE = 0.96;                
        } 
        linearCV = mul( SCALE, linearCV);
    }


    // Clip values < 0 (i.e. projecting outside the display primaries)
    // NOTE: P3 red and values close to it fall outside of Rec.2020 green-red 
    // boundary
    linearCV = clamp( linearCV, 0., HALF_POS_INF);

    // EOTF
    // 0: ST-2084 (PQ)
    // 1: BT.1886 (Rec.709/2020 settings)
    // 2: sRGB (mon_curve w/ presets)
    //    moncurve_r with gamma of 2.4 and offset of 0.055 matches the EOTF found in IEC 61966-2-1:1999 (sRGB)
    // 3: gamma 2.6
    // 4: linear (no EOTF)
    // 5: HLG
    float3 outputCV;
    if (EOTF == 0) {  // ST-2084 (PQ)
        // NOTE: This is a kludgy way of ensuring a PQ code value of 0. Ideally,
        // luminance would map directly to code value, but colorists don't like
        // that. Might just need the tonescale to go darker so that darkest
        // values through the tone scale quantize to code value of 0.
        if (STRETCH_BLACK == true) {
            outputCV = Y_2_ST2084_f3( clamp( linCV_2_Y_f3(linearCV, Y_MAX, 0.0), 0.0, HALF_POS_INF) );
        } else {
            outputCV = Y_2_ST2084_f3( linCV_2_Y_f3(linearCV, Y_MAX, Y_MIN) );        
        }
    } else if (EOTF == 1) { // BT.1886 (Rec.709/2020 settings)
        outputCV = bt1886_r_f3( linearCV, 2.4, 1.0, 0.0);
    } else if (EOTF == 2) { // sRGB (mon_curve w/ presets)
        outputCV = moncurve_r_f3( linearCV, 2.4, 0.055);
    } else if (EOTF == 3) { // gamma 2.6
        outputCV = pow( linearCV, 1./2.6);
    } else if (EOTF == 4) { // linear
        outputCV = linCV_2_Y_f3(linearCV, Y_MAX, Y_MIN);
    } else if (EOTF == 5) { // HLG
        // NOTE: HLG just maps ST.2084 output to HLG encoding. 
        // TODO: Restructure if/else tree to minimize this redundancy.
        if (STRETCH_BLACK == true) {
            outputCV = Y_2_ST2084_f3( clamp( linCV_2_Y_f3(linearCV, Y_MAX, 0.0), 0.0, HALF_POS_INF) );
        }
        else {
            outputCV = Y_2_ST2084_f3( linCV_2_Y_f3(linearCV, Y_MAX, Y_MIN) );        
        }
        outputCV = ST2084_2_HLG_1000nits_f3( outputCV);
    }

    if (LEGAL_RANGE == true) {
        outputCV = fullRange_to_smpteRange_f3( outputCV);
    }

    return outputCV;    
}

float3 invOutputTransform(
    float3 input,
    float Y_MIN,
    float Y_MID,
    float Y_MAX,    
    Chromaticities DISPLAY_PRI,
    Chromaticities LIMITING_PRI,
    int EOTF,  
    int SURROUND,
    bool STRETCH_BLACK = true,
    bool D60_SIM = false,
    bool LEGAL_RANGE = false
)
{
    float4x4 DISPLAY_PRI_2_XYZ_MAT = RGBtoXYZ( DISPLAY_PRI, 1.0);

    /* 
        NOTE: This is a bit of a hack - probably a more direct way to do this.
        TODO: Update in accordance with forward algorithm.
    */
    TsParams PARAMS_DEFAULT = init_TsParams( Y_MIN, Y_MAX);
    float expShift = log2(inv_ssts(Y_MID, PARAMS_DEFAULT))-log2(0.18);
    TsParams PARAMS = init_TsParams( Y_MIN, Y_MAX, expShift);

    float3 outputCV = input;

    if (LEGAL_RANGE == true) {
        outputCV = smpteRange_to_fullRange_f3( outputCV);
    }

    // Inverse EOTF
    // 0: ST-2084 (PQ)
    // 1: BT.1886 (Rec.709/2020 settings)
    // 2: sRGB (mon_curve w/ presets)
    //    moncurve_r with gamma of 2.4 and offset of 0.055 matches the EOTF found in IEC 61966-2-1:1999 (sRGB)
    // 3: gamma 2.6
    // 4: linear (no EOTF)
    // 5: HLG
    float3 linearCV;
    if (EOTF == 0) {  // ST-2084 (PQ)
        if (STRETCH_BLACK == true) {
            linearCV = Y_2_linCV_f3( ST2084_2_Y_f3( outputCV), Y_MAX, 0.);
        } else {
            linearCV = Y_2_linCV_f3( ST2084_2_Y_f3( outputCV), Y_MAX, Y_MIN);
        }
    } else if (EOTF == 1) { // BT.1886 (Rec.709/2020 settings)
        linearCV = bt1886_f_f3( outputCV, 2.4, 1.0, 0.0);
    } else if (EOTF == 2) { // sRGB (mon_curve w/ presets)
        linearCV = moncurve_f_f3( outputCV, 2.4, 0.055);
    } else if (EOTF == 3) { // gamma 2.6
        linearCV = pow( outputCV, 2.6);
    } else if (EOTF == 4) { // linear
        linearCV = Y_2_linCV_f3( outputCV, Y_MAX, Y_MIN);
    } else if (EOTF == 5) { // HLG
        outputCV = HLG_2_ST2084_1000nits_f3( outputCV);
        if (STRETCH_BLACK == true) {
            linearCV = Y_2_linCV_f3( ST2084_2_Y_f3( outputCV), Y_MAX, 0.);
        } else {
            linearCV = Y_2_linCV_f3( ST2084_2_Y_f3( outputCV), Y_MAX, Y_MIN);
        }
    }

    // Un-scale
    if (D60_SIM == true) {
        /* TODO: The scale requires calling itself. Need an algorithm for this.
            Scale is same no matter the luminance.
            Currently using precalculated values for D65, DCI.
            If DCI, roll_white_fwd is used also.
        */
        float SCALE = 1.0;
        if ((DISPLAY_PRI.white[0] == 0.3127) & 
            (DISPLAY_PRI.white[1] == 0.329)) { // D65
                SCALE = 0.96362;
                linearCV = mul( 1./SCALE, linearCV);
        } 
        else if ((DISPLAY_PRI.white[0] == 0.314) & 
                 (DISPLAY_PRI.white[1] == 0.351)) { // DCI
                SCALE = 0.96;                
                linearCV[0] = roll_white_rev( linearCV[0]/SCALE, 0.918, 0.5);
                linearCV[1] = roll_white_rev( linearCV[1]/SCALE, 0.918, 0.5);
                linearCV[2] = roll_white_rev( linearCV[2]/SCALE, 0.918, 0.5);
        } 

    }    

    // Encoding primaries to CIE XYZ
    float3 XYZ = mult_f3_f44( linearCV, DISPLAY_PRI_2_XYZ_MAT);

    // Undo CAT from assumed observer adapted white point to ACES white point
    if (D60_SIM == false) {
        if ((DISPLAY_PRI.white[0] != AP0.white[0]) &
            (DISPLAY_PRI.white[1] != AP0.white[1])) {
            float3x3 CAT = calculate_cat_matrix( AP0.white, DISPLAY_PRI.white);
            XYZ = mult_f3_f33( XYZ, inverse(D60_2_D65_CAT) );
        }
    }

    // Apply gamma adjustment to compensate for dim surround
    /*  
        NOTE: This is more or less a placeholder block and is largely inactive 
        in its current form. This section currently only applies for SDR, and
        even then, only in very specific cases.
        In the future it is fully intended for this module to be updated to 
        support surround compensation regardless of luminance dynamic range. */
    /*  
        TOD0: Come up with new surround compensation algorithm, applicable 
        across all dynamic ranges and supporting dark/dim/normal surround.  
    */
    if (SURROUND == 0) { // Dark surround
        /*  
        Current tone scale is designed for dark surround environment so no 
        adjustment is necessary. 
        */
    } else if (SURROUND == 1) { // Dim surround
        // INACTIVE for HDR and crudely implemented for SDR (see comment below)        
        if ((EOTF == 1) || (EOTF == 2) || (EOTF == 3)) { 
            /* 
            This uses a crude logical assumption that if the EOTF is BT.1886,
            sRGB, or gamma 2.6 that the data is SDR and so the SDR gamma
            compensation factor from v1.0 will apply. 
            */
            XYZ = dim_to_dark( XYZ); /*
            This uses a local dim_to_dark function that is designed to take in
            XYZ and return XYZ rather than AP1 as is currently in the functions
            in 'ACESlib.ODT_Common.ctl' */
        }
    } else if (SURROUND == 2) { // Normal surround
        // INACTIVE - this does NOTHING
    }

    // XYZ to rendering primaries
    linearCV = mult_f3_f44( XYZ, XYZ_2_AP1_MAT);

    float3 rgbPost = linCV_2_Y_f3( linearCV, Y_MAX, Y_MIN);

    // Apply the inverse tonescale independently in rendering-space RGB
    float3 rgbPre = inv_ssts_f3( rgbPost, PARAMS);

    // RRT sweeteners
    float3 aces = inv_rrt_sweeteners( rgbPre);

    return aces;
}

#endif
#ifndef __aces_constants__
#define __aces_constants__

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
#include "matrix_utils.hlsl"
#include "aces_utilities_color.hlsl"

#define M_PI 3.141593f
static const float FLT_NAN = 0x7fffffff;
static const min16float HALF_POS_INF = 0x7c00; 
static const min16float HALF_MAX = 65504.0f;
static const min16float HALF_MIN = 5.96046448e-08f;

float4x4 RGBtoXYZ(Chromaticities chroma, float Y)
{
    //
    // X and Z values of RGB value (1, 1, 1), or "white"
    //

    float X = chroma.white.x * Y / chroma.white.y;
    float Z = (1 - chroma.white.x - chroma.white.y) * Y / chroma.white.y;

    //
    // Scale factors for matrix rows
    //

    float d = chroma.red.x   * (chroma.blue.y  - chroma.green.y) +
        chroma.blue.x  * (chroma.green.y - chroma.red.y) +
        chroma.green.x * (chroma.red.y   - chroma.blue.y);

    float Sr = (X * (chroma.blue.y - chroma.green.y) -
          chroma.green.x * (Y * (chroma.blue.y - 1) +
    chroma.blue.y  * (X + Z)) +
    chroma.blue.x  * (Y * (chroma.green.y - 1) +
    chroma.green.y * (X + Z))) / d;

    float Sg = (X * (chroma.red.y - chroma.blue.y) +
    chroma.red.x   * (Y * (chroma.blue.y - 1) +
    chroma.blue.y  * (X + Z)) -
    chroma.blue.x  * (Y * (chroma.red.y - 1) +
    chroma.red.y   * (X + Z))) / d;

    float Sb = (X * (chroma.green.y - chroma.red.y) -
    chroma.red.x   * (Y * (chroma.green.y - 1) +
    chroma.green.y * (X + Z)) +
    chroma.green.x * (Y * (chroma.red.y - 1) +
    chroma.red.y   * (X + Z))) / d;

    //
    // Assemble the matrix
    //

    float4x4 M = _identity;

    M._m00 = Sr * chroma.red.x;
    M._m01 = Sr * chroma.red.y;
    M._m02 = Sr * (1 - chroma.red.x - chroma.red.y);

    M._m10 = Sg * chroma.green.x;
    M._m11 = Sg * chroma.green.y;
    M._m12 = Sg * (1 - chroma.green.x - chroma.green.y);

    M._m20 = Sb * chroma.blue.x;
    M._m21 = Sb * chroma.blue.y;
    M._m22 = Sb * (1 - chroma.blue.x - chroma.blue.y);

    return M;
}

float4x4 XYZtoRGB(Chromaticities chroma, float Y)
{
    return inverse(RGBtoXYZ(chroma, Y));
}

static const float4x4 AP0_2_XYZ_MAT = RGBtoXYZ( AP0, 1.0);
static const float4x4 XYZ_2_AP0_MAT = XYZtoRGB( AP0, 1.0);

static const float4x4 AP1_2_XYZ_MAT = RGBtoXYZ( AP1, 1.0);
static const float4x4 XYZ_2_AP1_MAT = XYZtoRGB( AP1, 1.0);

static const float4x4 AP0_2_AP1_MAT = mult_f44_f44( AP0_2_XYZ_MAT, XYZ_2_AP1_MAT);
static const float4x4 AP1_2_AP0_MAT = mult_f44_f44( AP1_2_XYZ_MAT, XYZ_2_AP0_MAT);

static const float3 AP1_RGB2Y = { AP1_2_XYZ_MAT._m01, 
                                  AP1_2_XYZ_MAT._m11, 
                                  AP1_2_XYZ_MAT._m21 };

#endif
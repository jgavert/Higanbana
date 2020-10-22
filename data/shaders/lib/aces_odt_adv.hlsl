#ifndef __aces_odt_adv__
#define __aces_odt_adv__

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
#include "aces_output_transforms.hlsl"


float4 odtHDR(float4 OCES, float3 luminance) {
  const Chromaticities DISPLAY_PRI = REC2020_PRI; // encoding primaries (device setup)
  const Chromaticities LIMITING_PRI = REC2020_PRI;// limiting primaries
  const int EOTF = 0;                           // 0: ST-2084 (PQ)
                                                // 1: BT.1886 (Rec.709/2020 settings) 
                                                // 2: sRGB (mon_curve w/ presets)
                                                // 3: gamma 2.6
                                                // 4: linear (no EOTF)
                                                // 5: HLG
  const int SURROUND = 0;                       // 0: dark ( NOTE: this is the only active setting! )
                                                // 1: dim ( *inactive* - selecting this will have no effect )
                                                // 2: normal ( *inactive* - selecting this will have no effect )
  const bool STRETCH_BLACK = true;                // stretch black luminance to a PQ code value of 0
  const bool D60_SIM = false;                       
  const bool LEGAL_RANGE = false;

  float3 cv = outputTransform( OCES.rgb, luminance.x, // black luminance (cd/m^2)
                                       luminance.y, // mid-point luminance (cd/m^2)
                                       luminance.z, // peak white luminance (cd/m^2)
                                       DISPLAY_PRI,
                                       LIMITING_PRI,
                                       EOTF,
                                       SURROUND,
                                       STRETCH_BLACK,
                                       D60_SIM,
                                       LEGAL_RANGE );
  return float4(cv, OCES.a);
}

static const float3 sdrLuminance = float3(0.0005, 0.02, 100);

float4 odtSDR(float4 OCES) {
  const Chromaticities DISPLAY_PRI = REC709_PRI; // encoding primaries (device setup)
  const Chromaticities LIMITING_PRI = REC709_PRI;// limiting primaries
  const int EOTF = 2;                           // 0: ST-2084 (PQ)
                                                // 1: BT.1886 (Rec.709/2020 settings) 
                                                // 2: sRGB (mon_curve w/ presets)
                                                // 3: gamma 2.6
                                                // 4: linear (no EOTF)
                                                // 5: HLG
  const int SURROUND = 0;                       // 0: dark ( NOTE: this is the only active setting! )
                                                // 1: dim ( *inactive* - selecting this will have no effect )
                                                // 2: normal ( *inactive* - selecting this will have no effect )
  const bool STRETCH_BLACK = true;                // stretch black luminance to a PQ code value of 0
  const bool D60_SIM = false;                       
  const bool LEGAL_RANGE = false;

  float3 cv = outputTransform( OCES.rgb, sdrLuminance.x, // black luminance (cd/m^2)
                                         sdrLuminance.y, // mid-point luminance (cd/m^2)
                                         sdrLuminance.z, // peak white luminance (cd/m^2)
                                       DISPLAY_PRI,
                                       LIMITING_PRI,
                                       EOTF,
                                       SURROUND,
                                       STRETCH_BLACK,
                                       D60_SIM,
                                       LEGAL_RANGE );
  return float4(cv, OCES.a);
}

float4 invOdtSDR(float4 SDR) {
  const Chromaticities DISPLAY_PRI = REC709_PRI; // encoding primaries (device setup)
  const Chromaticities LIMITING_PRI = REC709_PRI;// limiting primaries
  const int EOTF = 2;                           // 0: ST-2084 (PQ)
                                                // 1: BT.1886 (Rec.709/2020 settings) 
                                                // 2: sRGB (mon_curve w/ presets)
                                                // 3: gamma 2.6
                                                // 4: linear (no EOTF)
                                                // 5: HLG
  const int SURROUND = 0;                       // 0: dark ( NOTE: this is the only active setting! )
                                                // 1: dim ( *inactive* - selecting this will have no effect )
                                                // 2: normal ( *inactive* - selecting this will have no effect )
  const bool STRETCH_BLACK = true;                // stretch black luminance to a PQ code value of 0
  const bool D60_SIM = false;                       
  const bool LEGAL_RANGE = false;

  float3 cv = invOutputTransform( SDR.rgb, sdrLuminance.x, // black luminance (cd/m^2)
                                           sdrLuminance.y, // mid-point luminance (cd/m^2)
                                           sdrLuminance.z, // peak white luminance (cd/m^2)
                                       DISPLAY_PRI,
                                       LIMITING_PRI,
                                       EOTF,
                                       SURROUND,
                                       STRETCH_BLACK,
                                       D60_SIM,
                                       LEGAL_RANGE );
  return float4(cv, SDR.a);
}

#endif
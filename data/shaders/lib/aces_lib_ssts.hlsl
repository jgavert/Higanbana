#ifndef __aces_lib_ssts__
#define __aces_lib_ssts__

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

//
// Contains functions used for forward and inverse tone scale 
//

float interpolate1D_22(
  const float table[2][2],
  //int size,
  float p)
{
  int size = 2;
  if (size < 1)
    return 0;

  if (p < table[0][0])
    return table[0][1];

  if (p >= table[size-1][0])
    return table[size-1][1];

  int i = 0;
  int j = size;

  while (i < j - 1)
  {
    int k = (i + j) / 2;

    if (table[k][0] == p)
      return table[k][1];
    else if (table[k][0] < p)
      i = k;
    else
      j = k;
  }

    float t = (p - table[i][0]) / (table[i+1][0] - table[i][0]);
    float s = 1 - t;

    return s * table[i][1] + t * table[i+1][1];
}


// Textbook monomial to basis-function conversion matrix.
static const float3x3 M1 = {
  {  0.5, -1.0, 0.5 },
  { -1.0,  1.0, 0.5 },
  {  0.5,  0.0, 0.0 }
};

struct TsPoint
{
    float x;        // ACES
    float y;        // luminance
    float slope;    // 
};

struct TsParams
{
    TsPoint Min;
    TsPoint Mid;
    TsPoint Max;
    float coefsLow[6];
    float coefsHigh[6];    
};

struct Array5
{
  float arr[5];
};

// TODO: Move all "magic numbers" (i.e. values in interpolation tables, etc.) to top 
// and define as constants

static const float MIN_STOP_SDR = -6.5;
static const float MAX_STOP_SDR = 6.5;

static const float MIN_STOP_RRT = -15.;
static const float MAX_STOP_RRT = 18.;

static const float MIN_LUM_SDR = 0.02;
static const float MAX_LUM_SDR = 48.0;

static const float MIN_LUM_RRT = 0.0001;
static const float MAX_LUM_RRT = 10000.0;


float lookup_ACESmin( float minLum )
{
    const float minTable[2][2] = { { log10(MIN_LUM_RRT), MIN_STOP_RRT }, 
                                   { log10(MIN_LUM_SDR), MIN_STOP_SDR } };

    return 0.18*pow( 2., interpolate1D_22( minTable, log10( minLum)));
}

float lookup_ACESmax( float maxLum )
{
    const float maxTable[2][2] = { { log10(MAX_LUM_SDR), MAX_STOP_SDR }, 
                                   { log10(MAX_LUM_RRT), MAX_STOP_RRT } };

    return 0.18*pow( 2., interpolate1D_22( maxTable, log10( maxLum)));
}

Array5 init_coefsLow(
    TsPoint TsPointLow,
    TsPoint TsPointMid)
{
    float coefsLow[5];

    float knotIncLow = (log10(TsPointMid.x) - log10(TsPointLow.x)) / 3.;
    // float halfKnotInc = (log10(TsPointMid.x) - log10(TsPointLow.x)) / 6.;

    // Determine two lowest coefficients (straddling minPt)
    coefsLow[0] = (TsPointLow.slope * (log10(TsPointLow.x)-0.5*knotIncLow)) + ( log10(TsPointLow.y) - TsPointLow.slope * log10(TsPointLow.x));
    coefsLow[1] = (TsPointLow.slope * (log10(TsPointLow.x)+0.5*knotIncLow)) + ( log10(TsPointLow.y) - TsPointLow.slope * log10(TsPointLow.x));
    // NOTE: if slope=0, then the above becomes just 
        // coefsLow[0] = log10(TsPointLow.y);
        // coefsLow[1] = log10(TsPointLow.y);
    // leaving it as a variable for now in case we decide we need non-zero slope extensions

    // Determine two highest coefficients (straddling midPt)
    coefsLow[3] = (TsPointMid.slope * (log10(TsPointMid.x)-0.5*knotIncLow)) + ( log10(TsPointMid.y) - TsPointMid.slope * log10(TsPointMid.x));
    coefsLow[4] = (TsPointMid.slope * (log10(TsPointMid.x)+0.5*knotIncLow)) + ( log10(TsPointMid.y) - TsPointMid.slope * log10(TsPointMid.x));
    
    // Middle coefficient (which defines the "sharpness of the bend") is linearly interpolated
    float bendsLow[2][2] = { {MIN_STOP_RRT, 0.18}, 
                             {MIN_STOP_SDR, 0.35} };
    float pctLow = interpolate1D_22( bendsLow, log2(TsPointLow.x/0.18));
    coefsLow[2] = log10(TsPointLow.y) + pctLow*(log10(TsPointMid.y)-log10(TsPointLow.y));
    Array5 ret;
    ret.arr = coefsLow;
    return ret;
} 

Array5 init_coefsHigh( 
    TsPoint TsPointMid, 
    TsPoint TsPointMax
)
{
    float coefsHigh[5];

    float knotIncHigh = (log10(TsPointMax.x) - log10(TsPointMid.x)) / 3.;
    // float halfKnotInc = (log10(TsPointMax.x) - log10(TsPointMid.x)) / 6.;

    // Determine two lowest coefficients (straddling midPt)
    coefsHigh[0] = (TsPointMid.slope * (log10(TsPointMid.x)-0.5*knotIncHigh)) + ( log10(TsPointMid.y) - TsPointMid.slope * log10(TsPointMid.x));
    coefsHigh[1] = (TsPointMid.slope * (log10(TsPointMid.x)+0.5*knotIncHigh)) + ( log10(TsPointMid.y) - TsPointMid.slope * log10(TsPointMid.x));

    // Determine two highest coefficients (straddling maxPt)
    coefsHigh[3] = (TsPointMax.slope * (log10(TsPointMax.x)-0.5*knotIncHigh)) + ( log10(TsPointMax.y) - TsPointMax.slope * log10(TsPointMax.x));
    coefsHigh[4] = (TsPointMax.slope * (log10(TsPointMax.x)+0.5*knotIncHigh)) + ( log10(TsPointMax.y) - TsPointMax.slope * log10(TsPointMax.x));
    // NOTE: if slope=0, then the above becomes just
        // coefsHigh[0] = log10(TsPointHigh.y);
        // coefsHigh[1] = log10(TsPointHigh.y);
    // leaving it as a variable for now in case we decide we need non-zero slope extensions
    
    // Middle coefficient (which defines the "sharpness of the bend") is linearly interpolated
    float bendsHigh[2][2] = { {MAX_STOP_SDR, 0.89}, 
                              {MAX_STOP_RRT, 0.90} };
    float pctHigh = interpolate1D_22( bendsHigh, log2(TsPointMax.x/0.18));
    coefsHigh[2] = log10(TsPointMid.y) + pctHigh*(log10(TsPointMax.y)-log10(TsPointMid.y));
    
    Array5 ret;
    ret.arr = coefsHigh;
    return ret;
}


float shift( float input, float expShift)
{
    return pow(2.,(log2(input)-expShift));
}


TsParams init_TsParams(
    float minLum,
    float maxLum,
    float expShift = 0
)
{
    TsPoint MIN_PT = { lookup_ACESmin(minLum), minLum, 0.0};
    TsPoint MID_PT = { 0.18, 4.8, 1.55};
    TsPoint MAX_PT = { lookup_ACESmax(maxLum), maxLum, 0.0};
    float cLow[5] = init_coefsLow( MIN_PT, MID_PT);
    float cHigh[5] = init_coefsHigh( MID_PT, MAX_PT);
    MIN_PT.x = shift(lookup_ACESmin(minLum),expShift);
    MID_PT.x = shift(0.18,expShift);
    MAX_PT.x = shift(lookup_ACESmax(maxLum),expShift);

    TsParams P = {
        {MIN_PT.x, MIN_PT.y, MIN_PT.slope},
        {MID_PT.x, MID_PT.y, MID_PT.slope},
        {MAX_PT.x, MAX_PT.y, MAX_PT.slope},
        {cLow[0], cLow[1], cLow[2], cLow[3], cLow[4], cLow[4]},
        {cHigh[0], cHigh[1], cHigh[2], cHigh[3], cHigh[4], cHigh[4]}
    };
         
    return P;
}


float ssts( 
    float x,
    TsParams C)
{
    const int N_KNOTS_LOW = 4;
    const int N_KNOTS_HIGH = 4;

    // Check for negatives or zero before taking the log. If negative or zero,
    // set to HALF_MIN.
    float logx = log10( max(x, HALF_MIN )); 

    float logy;

    if ( logx <= log10(C.Min.x) ) { 

        logy = logx * C.Min.slope + ( log10(C.Min.y) - C.Min.slope * log10(C.Min.x) );

    } else if (( logx > log10(C.Min.x) ) && ( logx < log10(C.Mid.x) )) {

        float knot_coord = (N_KNOTS_LOW-1) * (logx-log10(C.Min.x))/(log10(C.Mid.x)-log10(C.Min.x));
        int j = knot_coord;
        float t = knot_coord - j;

        float3 cf = { C.coefsLow[ j], C.coefsLow[ j + 1], C.coefsLow[ j + 2]};

        float3 monomials = { t * t, t, 1. };
        logy = dot( monomials, mult_f3_f33( cf, M1));

    } else if (( logx >= log10(C.Mid.x) ) && ( logx < log10(C.Max.x) )) {

        float knot_coord = (N_KNOTS_HIGH-1) * (logx-log10(C.Mid.x))/(log10(C.Max.x)-log10(C.Mid.x));
        int j = knot_coord;
        float t = knot_coord - j;

        float3 cf = { C.coefsHigh[ j], C.coefsHigh[ j + 1], C.coefsHigh[ j + 2]}; 

        float3 monomials = { t * t, t, 1. };
        logy = dot( monomials, mult_f3_f33( cf, M1));

    } else { //if ( logIn >= log10(C.Max.x) ) { 

        logy = logx * C.Max.slope + ( log10(C.Max.y) - C.Max.slope * log10(C.Max.x) );

    }

    return pow10(logy);

}


float inv_ssts( 
    float y,
    TsParams C)
{  
    const int N_KNOTS_LOW = 4;
    const int N_KNOTS_HIGH = 4;

    const float KNOT_INC_LOW = (log10(C.Mid.x) - log10(C.Min.x)) / (N_KNOTS_LOW - 1.);
    const float KNOT_INC_HIGH = (log10(C.Max.x) - log10(C.Mid.x)) / (N_KNOTS_HIGH - 1.);

    // KNOT_Y is luminance of the spline at each knot
    float KNOT_Y_LOW[ N_KNOTS_LOW];
    for (int i = 0; i < N_KNOTS_LOW; i = i+1) {
    KNOT_Y_LOW[ i] = ( C.coefsLow[i] + C.coefsLow[i+1]) / 2.;
    };

    float KNOT_Y_HIGH[ N_KNOTS_HIGH];
    for (int ii = 0; ii < N_KNOTS_HIGH; ii = ii+1) {
    KNOT_Y_HIGH[ ii] = ( C.coefsHigh[ii] + C.coefsHigh[ii+1]) / 2.;
    };

    float logy = log10( max(y,1e-10));

    float logx;
    if (logy <= log10(C.Min.y)) {

        logx = log10(C.Min.x);

    } else if ( (logy > log10(C.Min.y)) && (logy <= log10(C.Mid.y)) ) {

        unsigned int j;
        float3 cf;
        if ( logy > KNOT_Y_LOW[ 0] && logy <= KNOT_Y_LOW[ 1]) {
            cf[ 0] = C.coefsLow[0];  cf[ 1] = C.coefsLow[1];  cf[ 2] = C.coefsLow[2];  j = 0;
        } else if ( logy > KNOT_Y_LOW[ 1] && logy <= KNOT_Y_LOW[ 2]) {
            cf[ 0] = C.coefsLow[1];  cf[ 1] = C.coefsLow[2];  cf[ 2] = C.coefsLow[3];  j = 1;
        } else if ( logy > KNOT_Y_LOW[ 2] && logy <= KNOT_Y_LOW[ 3]) {
            cf[ 0] = C.coefsLow[2];  cf[ 1] = C.coefsLow[3];  cf[ 2] = C.coefsLow[4];  j = 2;
        } 

        const float3 tmp = mult_f3_f33( cf, M1);

        float a = tmp[ 0];
        float b = tmp[ 1];
        float c = tmp[ 2];
        c = c - logy;

        const float d = sqrt( b * b - 4. * a * c);

        const float t = ( 2. * c) / ( -d - b);

        logx = log10(C.Min.x) + ( t + j) * KNOT_INC_LOW;

    } else if ( (logy > log10(C.Mid.y)) && (logy < log10(C.Max.y)) ) {

        unsigned int j;
        float3 cf;
        if ( logy >= KNOT_Y_HIGH[ 0] && logy <= KNOT_Y_HIGH[ 1]) {
            cf[ 0] = C.coefsHigh[0];  cf[ 1] = C.coefsHigh[1];  cf[ 2] = C.coefsHigh[2];  j = 0;
        } else if ( logy > KNOT_Y_HIGH[ 1] && logy <= KNOT_Y_HIGH[ 2]) {
            cf[ 0] = C.coefsHigh[1];  cf[ 1] = C.coefsHigh[2];  cf[ 2] = C.coefsHigh[3];  j = 1;
        } else if ( logy > KNOT_Y_HIGH[ 2] && logy <= KNOT_Y_HIGH[ 3]) {
            cf[ 0] = C.coefsHigh[2];  cf[ 1] = C.coefsHigh[3];  cf[ 2] = C.coefsHigh[4];  j = 2;
        } 

        const float3 tmp = mult_f3_f33( cf, M1);

        float a = tmp[ 0];
        float b = tmp[ 1];
        float c = tmp[ 2];
        c = c - logy;

        const float d = sqrt( b * b - 4. * a * c);

        const float t = ( 2. * c) / ( -d - b);

        logx = log10(C.Mid.x) + ( t + j) * KNOT_INC_HIGH;

    } else { //if ( logy >= log10(C.Max.y) ) {

        logx = log10(C.Max.x);

    }

    return pow10( logx);

}


float3 ssts_f3( 
    float3 x,
    TsParams C)
{
    float3 output;
    output[0] = ssts( x[0], C);
    output[1] = ssts( x[1], C);
    output[2] = ssts( x[2], C);

    return output;
}


float3 inv_ssts_f3( 
    float3 x,
    TsParams C)
{
    float3 output;
    output[0] = inv_ssts( x[0], C);
    output[1] = inv_ssts( x[1], C);
    output[2] = inv_ssts( x[2], C);

    return output;
}

#endif
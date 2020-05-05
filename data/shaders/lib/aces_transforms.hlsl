#ifndef __aces_transform_common__
#define __aces_transform_common__

#include "aces_constants.hlsl"

float max3(float3 val) {
  return max(max(val.r, val.g), val.b);
}
float min3(float3 val) {
  return min(min(val.r, val.g), val.b);
}

float pow10(float x) {
  return pow(10,x);
}

float rgb_2_saturation(float3 rgb)
{
  return ( max( max3(rgb), TINY) - max( min3(rgb), TINY)) / max( max3(rgb), 1e-2);
}

// utilities
float rgb_2_hue( float3 rgb) {
  // Returns a geometric hue angle in degrees (0-360) based on RGB values.
  // For neutral colors, hue is undefined and the function will return a quiet NaN value.
  float hue;
  if (rgb[0] == rgb[1] && rgb[1] == rgb[2]) {
    hue = FLT_NAN; // RGB triplets where RGB are equal have an undefined hue
  } else {
    hue = (180./M_PI) * atan2( sqrt(3)*(rgb[1]-rgb[2]), 2*rgb[0]-rgb[1]-rgb[2]);
  }
    
  if (hue < 0.) hue = hue + 360.;

  return hue;
}

//static const float defaultycRadiusWeight = 1.75f; 
float rgb_2_yc(float3 rgb, float ycRadiusWeight = 1.75)
{
  // Converts RGB to a luminance proxy, here called YC
  // YC is ~ Y + K * Chroma
  // Constant YC is a cone-shaped surface in RGB space, with the tip on the 
  // neutral axis, towards white.
  // YC is normalized: RGB 1 1 1 maps to YC = 1
  //
  // ycRadiusWeight defaults to 1.75, although can be overridden in function 
  // call to rgb_2_yc
  // ycRadiusWeight = 1 -> YC for pure cyan, magenta, yellow == YC for neutral 
  // of same value
  // ycRadiusWeight = 2 -> YC for pure red, green, blue  == YC for  neutral of 
  // same value.
  
  float chroma = sqrt(rgb.b*(rgb.b-rgb.g)+rgb.g*(rgb.g-rgb.r)+rgb.r*(rgb.r-rgb.b));

  return ( rgb.b + rgb.g + rgb.r + ycRadiusWeight * chroma) / 3.f;
}

float3x3 calc_sat_adjust_matrix(float sat, float3 rgb2Y) {
  //
  // This function determines the terms for a 3x3 saturation matrix that is
  // based on the luminance of the input.
  //
  float3x3 M;
  M[0][0] = (1.0 - sat) * rgb2Y[0] + sat;
  M[1][0] = (1.0 - sat) * rgb2Y[0];
  M[2][0] = (1.0 - sat) * rgb2Y[0];
  
  M[0][1] = (1.0 - sat) * rgb2Y[1];
  M[1][1] = (1.0 - sat) * rgb2Y[1] + sat;
  M[2][1] = (1.0 - sat) * rgb2Y[1];
  
  M[0][2] = (1.0 - sat) * rgb2Y[2];
  M[1][2] = (1.0 - sat) * rgb2Y[2];
  M[2][2] = (1.0 - sat) * rgb2Y[2] + sat;

  M = transpose(M);    
  return M;
} 

// tonescales

// Textbook monomial to basis-function conversion matrix.
static const float3x3 M = float3x3(
   0.5, -1.0, 0.5,
  -1.0,  1.0, 0.5,
   0.5,  0.0, 0.0
);



struct SplineMapPoint
{
  float x;
  float y;
};

struct SegmentedSplineParams_c5
{
  float coefsLow[6];    // coefs for B-spline between minPoint and midPoint (units of log luminance)
  float coefsHigh[6];   // coefs for B-spline between midPoint and maxPoint (units of log luminance)
  SplineMapPoint minPoint; // {luminance, luminance} linear extension below this
  SplineMapPoint midPoint; // {luminance, luminance} 
  SplineMapPoint maxPoint; // {luminance, luminance} linear extension above this
  float slopeLow;       // log-log slope of low linear extension
  float slopeHigh;      // log-log slope of high linear extension
};

struct SegmentedSplineParams_c9
{
  float coefsLow[10];    // coefs for B-spline between minPoint and midPoint (units of log luminance)
  float coefsHigh[10];   // coefs for B-spline between midPoint and maxPoint (units of log luminance)
  SplineMapPoint minPoint; // {luminance, luminance} linear extension below this
  SplineMapPoint midPoint; // {luminance, luminance} 
  SplineMapPoint maxPoint; // {luminance, luminance} linear extension above this
  float slopeLow;       // log-log slope of low linear extension
  float slopeHigh;      // log-log slope of high linear extension
};

static const SegmentedSplineParams_c5 RRT_PARAMS =
{
  // coefsLow[6]
  { -4.0000000000, -4.0000000000, -3.1573765773, -0.4852499958, 1.8477324706, 1.8477324706 },
  // coefsHigh[6]
  { -0.7185482425, 2.0810307172, 3.6681241237, 4.0000000000, 4.0000000000, 4.0000000000 },
  { 0.18*pow(2.,-15), 0.0001},    // minPoint
  { 0.18,                4.8},    // midPoint  
  { 0.18*pow(2., 18), 10000.},    // maxPoint
  0.0,  // slopeLow
  0.0   // slopeHigh
};

float segmented_spline_c5_fwd(float x, SegmentedSplineParams_c5 C = RRT_PARAMS) {
  const int N_KNOTS_LOW = 4;
  const int N_KNOTS_HIGH = 4;

  // Check for negatives or zero before taking the log. If negative or zero,
  // set to HALF_MIN.
  float logx = log10( max(x, HALF_MIN )); 

  float logy;

  if ( logx <= log10(C.minPoint.x) ) { 

    logy = logx * C.slopeLow + ( log10(C.minPoint.y) - C.slopeLow * log10(C.minPoint.x) );

  } else if (( logx > log10(C.minPoint.x) ) && ( logx < log10(C.midPoint.x) )) {

    float knot_coord = (N_KNOTS_LOW-1) * (logx-log10(C.minPoint.x))/(log10(C.midPoint.x)-log10(C.minPoint.x));
    int j = knot_coord;
    float t = knot_coord - j;

    float3 cf = { C.coefsLow[ j], C.coefsLow[ j + 1], C.coefsLow[ j + 2]};
    // NOTE: If the running a version of CTL < 1.5, you may get an 
    // exception thrown error, usually accompanied by "Array index out of range" 
    // If you receive this error, it is recommended that you update to CTL v1.5, 
    // which contains a number of important bug fixes. Otherwise, you may try 
    // uncommenting the below, which is longer, but equivalent to, the above 
    // line of code.
    //
    // float cf[ 3];
    // if ( j <= 0) {
    //     cf[ 0] = C.coefsLow[0];  cf[ 1] = C.coefsLow[1];  cf[ 2] = C.coefsLow[2];
    // } else if ( j == 1) {
    //     cf[ 0] = C.coefsLow[1];  cf[ 1] = C.coefsLow[2];  cf[ 2] = C.coefsLow[3];
    // } else if ( j == 2) {
    //     cf[ 0] = C.coefsLow[2];  cf[ 1] = C.coefsLow[3];  cf[ 2] = C.coefsLow[4];
    // } else if ( j == 3) {
    //     cf[ 0] = C.coefsLow[3];  cf[ 1] = C.coefsLow[4];  cf[ 2] = C.coefsLow[5];
    // } else if ( j == 4) {
    //     cf[ 0] = C.coefsLow[4];  cf[ 1] = C.coefsLow[5];  cf[ 2] = C.coefsLow[6];
    // } else if ( j == 5) {
    //     cf[ 0] = C.coefsLow[5];  cf[ 1] = C.coefsLow[6];  cf[ 2] = C.coefsLow[7];
    // } else if ( j == 6) {
    //     cf[ 0] = C.coefsLow[6];  cf[ 1] = C.coefsLow[7];  cf[ 2] = C.coefsLow[8];
    // } 
    
    float3 monomials = float3(t * t, t, 1.);
    logy = dot( monomials, mult_f3_f33( cf, M));

  } else if (( logx >= log10(C.midPoint.x) ) && ( logx < log10(C.maxPoint.x) )) {

    float knot_coord = (N_KNOTS_HIGH-1) * (logx-log10(C.midPoint.x))/(log10(C.maxPoint.x)-log10(C.midPoint.x));
    int j = knot_coord;
    float t = knot_coord - j;

    float3 cf = { C.coefsHigh[ j], C.coefsHigh[ j + 1], C.coefsHigh[ j + 2]}; 
    // NOTE: If the running a version of CTL < 1.5, you may get an 
    // exception thrown error, usually accompanied by "Array index out of range" 
    // If you receive this error, it is recommended that you update to CTL v1.5, 
    // which contains a number of important bug fixes. Otherwise, you may try 
    // uncommenting the below, which is longer, but equivalent to, the above 
    // line of code.
    //
    // float cf[ 3];
    // if ( j <= 0) {
    //     cf[ 0] = C.coefsHigh[0];  cf[ 1] = C.coefsHigh[1];  cf[ 2] = C.coefsHigh[2];
    // } else if ( j == 1) {
    //     cf[ 0] = C.coefsHigh[1];  cf[ 1] = C.coefsHigh[2];  cf[ 2] = C.coefsHigh[3];
    // } else if ( j == 2) {
    //     cf[ 0] = C.coefsHigh[2];  cf[ 1] = C.coefsHigh[3];  cf[ 2] = C.coefsHigh[4];
    // } else if ( j == 3) {
    //     cf[ 0] = C.coefsHigh[3];  cf[ 1] = C.coefsHigh[4];  cf[ 2] = C.coefsHigh[5];
    // } else if ( j == 4) {
    //     cf[ 0] = C.coefsHigh[4];  cf[ 1] = C.coefsHigh[5];  cf[ 2] = C.coefsHigh[6];
    // } else if ( j == 5) {
    //     cf[ 0] = C.coefsHigh[5];  cf[ 1] = C.coefsHigh[6];  cf[ 2] = C.coefsHigh[7];
    // } else if ( j == 6) {
    //     cf[ 0] = C.coefsHigh[6];  cf[ 1] = C.coefsHigh[7];  cf[ 2] = C.coefsHigh[8];
    // } 

    float3 monomials = { t * t, t, 1. };
    logy = dot( monomials, mult_f3_f33( cf, M));

  } else { //if ( logIn >= log10(C.maxPoint.x) ) { 
    logy = logx * C.slopeHigh + ( log10(C.maxPoint.y) - C.slopeHigh * log10(C.maxPoint.x) );
  }
  return pow10(logy);
}

float segmented_spline_c5_rev(float y, SegmentedSplineParams_c5 C = RRT_PARAMS) {  
  static const int N_KNOTS_LOW = 4;
  static const int N_KNOTS_HIGH = 4;

  static const float KNOT_INC_LOW = (log10(C.midPoint.x) - log10(C.minPoint.x)) / (N_KNOTS_LOW - 1.);
  static const float KNOT_INC_HIGH = (log10(C.maxPoint.x) - log10(C.midPoint.x)) / (N_KNOTS_HIGH - 1.);
  
  // KNOT_Y is luminance of the spline at each knot
  float KNOT_Y_LOW[ N_KNOTS_LOW];
  for (int i = 0; i < N_KNOTS_LOW; i = i+1) {
    KNOT_Y_LOW[ i] = ( C.coefsLow[i] + C.coefsLow[i+1]) / 2.;
  };

  float KNOT_Y_HIGH[ N_KNOTS_HIGH];
  for (int k = 0; k < N_KNOTS_HIGH; k = k+1) {
    KNOT_Y_HIGH[k] = ( C.coefsHigh[k] + C.coefsHigh[k+1]) / 2.;
  };

  float logy = log10( max(y,1e-10));

  float logx;
  if (logy <= log10(C.minPoint.y)) {

    logx = log10(C.minPoint.x);

  } else if ( (logy > log10(C.minPoint.y)) && (logy <= log10(C.midPoint.y)) ) {

    unsigned int j;
    float3 cf;
    if ( logy > KNOT_Y_LOW[ 0] && logy <= KNOT_Y_LOW[ 1]) {
        cf[ 0] = C.coefsLow[0];  cf[ 1] = C.coefsLow[1];  cf[ 2] = C.coefsLow[2];  j = 0;
    } else if ( logy > KNOT_Y_LOW[ 1] && logy <= KNOT_Y_LOW[ 2]) {
        cf[ 0] = C.coefsLow[1];  cf[ 1] = C.coefsLow[2];  cf[ 2] = C.coefsLow[3];  j = 1;
    } else if ( logy > KNOT_Y_LOW[ 2] && logy <= KNOT_Y_LOW[ 3]) {
        cf[ 0] = C.coefsLow[2];  cf[ 1] = C.coefsLow[3];  cf[ 2] = C.coefsLow[4];  j = 2;
    } 
    
    const float3 tmp = mult_f3_f33( cf, M);

    float a = tmp[ 0];
    float b = tmp[ 1];
    float c = tmp[ 2];
    c = c - logy;

    const float d = sqrt( b * b - 4. * a * c);

    const float t = ( 2. * c) / ( -d - b);

    logx = log10(C.minPoint.x) + ( t + j) * KNOT_INC_LOW;

  } else if ( (logy > log10(C.midPoint.y)) && (logy < log10(C.maxPoint.y)) ) {

    unsigned int j;
    float3 cf;
    if ( logy > KNOT_Y_HIGH[ 0] && logy <= KNOT_Y_HIGH[ 1]) {
        cf[ 0] = C.coefsHigh[0];  cf[ 1] = C.coefsHigh[1];  cf[ 2] = C.coefsHigh[2];  j = 0;
    } else if ( logy > KNOT_Y_HIGH[ 1] && logy <= KNOT_Y_HIGH[ 2]) {
        cf[ 0] = C.coefsHigh[1];  cf[ 1] = C.coefsHigh[2];  cf[ 2] = C.coefsHigh[3];  j = 1;
    } else if ( logy > KNOT_Y_HIGH[ 2] && logy <= KNOT_Y_HIGH[ 3]) {
        cf[ 0] = C.coefsHigh[2];  cf[ 1] = C.coefsHigh[3];  cf[ 2] = C.coefsHigh[4];  j = 2;
    } 
    
    const float3 tmp = mult_f3_f33(cf, M);

    float a = tmp[ 0];
    float b = tmp[ 1];
    float c = tmp[ 2];
    c = c - logy;

    const float d = sqrt( b * b - 4. * a * c);

    const float t = ( 2. * c) / ( -d - b);

    logx = log10(C.midPoint.x) + ( t + j) * KNOT_INC_HIGH;

  } else { //if ( logy >= log10(C.maxPoint.y) ) {
    logx = log10(C.maxPoint.x);
  }
  return pow10( logx);
}


static const SegmentedSplineParams_c9 ODT_48nits =
{
  // coefsLow[10]
  { -1.6989700043, -1.6989700043, -1.4779000000, -1.2291000000, -0.8648000000, -0.4480000000, 0.0051800000, 0.4511080334, 0.9113744414, 0.9113744414},
  // coefsHigh[10]
  { 0.5154386965, 0.8470437783, 1.1358000000, 1.3802000000, 1.5197000000, 1.5985000000, 1.6467000000, 1.6746091357, 1.6878733390, 1.6878733390 },
  {segmented_spline_c5_fwd( 0.18*pow(2.,-6.5) ),  0.02},    // minPoint
  {segmented_spline_c5_fwd( 0.18 ),                4.8},    // midPoint  
  {segmented_spline_c5_fwd( 0.18*pow(2.,6.5) ),   48.0},    // maxPoint
  0.0,  // slopeLow
  0.04  // slopeHigh
};

static const SegmentedSplineParams_c9 ODT_1000nits =
{
  // coefsLow[10]
  { -4.9706219331, -3.0293780669, -2.1262, -1.5105, -1.0578, -0.4668, 0.11938, 0.7088134201, 1.2911865799, 1.2911865799 },
  // coefsHigh[10]
  { 0.8089132070, 1.1910867930, 1.5683, 1.9483, 2.3083, 2.6384, 2.8595, 2.9872608805, 3.0127391195, 3.0127391195 },
  {segmented_spline_c5_fwd( 0.18*pow(2.,-12.) ), 0.0001},    // minPoint
  {segmented_spline_c5_fwd( 0.18 ),                10.0},    // midPoint  
  {segmented_spline_c5_fwd( 0.18*pow(2.,10.) ),  1000.0},    // maxPoint
  3.0,  // slopeLow
  0.06  // slopeHigh
};

static const SegmentedSplineParams_c9 ODT_2000nits =
{
  // coefsLow[10]
  { -4.9706219331, -3.0293780669, -2.1262, -1.5105, -1.0578, -0.4668, 0.11938, 0.7088134201, 1.2911865799, 1.2911865799 },
  // coefsHigh[10]
  { 0.8019952042, 1.1980047958, 1.5943000000, 1.9973000000, 2.3783000000, 2.7684000000, 3.0515000000, 3.2746293562, 3.3274306351, 3.3274306351 },
  {segmented_spline_c5_fwd( 0.18*pow(2.,-12.) ), 0.0001},    // minPoint
  {segmented_spline_c5_fwd( 0.18 ),                10.0},    // midPoint  
  {segmented_spline_c5_fwd( 0.18*pow(2.,11.) ),  2000.0},    // maxPoint
  3.0,  // slopeLow
  0.12  // slopeHigh
};

static const SegmentedSplineParams_c9 ODT_4000nits =
{
  // coefsLow[10]
  { -4.9706219331, -3.0293780669, -2.1262, -1.5105, -1.0578, -0.4668, 0.11938, 0.7088134201, 1.2911865799, 1.2911865799 },
  // coefsHigh[10]
  { 0.7973186613, 1.2026813387, 1.6093000000, 2.0108000000, 2.4148000000, 2.8179000000, 3.1725000000, 3.5344995451, 3.6696204376, 3.6696204376 },
  {segmented_spline_c5_fwd( 0.18*pow(2.,-12.) ), 0.0001},    // minPoint
  {segmented_spline_c5_fwd( 0.18 ),                10.0},    // midPoint  
  {segmented_spline_c5_fwd( 0.18*pow(2.,12.) ),  4000.0},    // maxPoint
  3.0,  // slopeLow
  0.3   // slopeHigh
};

float segmented_spline_c9_fwd(float x, SegmentedSplineParams_c9 C = ODT_48nits) {    
  static const int N_KNOTS_LOW = 8;
  static const int N_KNOTS_HIGH = 8;

  // Check for negatives or zero before taking the log. If negative or zero,
  // set to HALF_MIN.
  float logx = log10( max(x, HALF_MIN )); 

  float logy;

  if ( logx <= log10(C.minPoint.x) ) { 

    logy = logx * C.slopeLow + ( log10(C.minPoint.y) - C.slopeLow * log10(C.minPoint.x) );

  } else if (( logx > log10(C.minPoint.x) ) && ( logx < log10(C.midPoint.x) )) {

    float knot_coord = (N_KNOTS_LOW-1) * (logx-log10(C.minPoint.x))/(log10(C.midPoint.x)-log10(C.minPoint.x));
    int j = knot_coord;
    float t = knot_coord - j;

    float3 cf = { C.coefsLow[ j], C.coefsLow[ j + 1], C.coefsLow[ j + 2]};
    // NOTE: If the running a version of CTL < 1.5, you may get an 
    // exception thrown error, usually accompanied by "Array index out of range" 
    // If you receive this error, it is recommended that you update to CTL v1.5, 
    // which contains a number of important bug fixes. Otherwise, you may try 
    // uncommenting the below, which is longer, but equivalent to, the above 
    // line of code.
    //
    // float3 cf;
    // if ( j <= 0) {
    //     cf[ 0] = C.coefsLow[0];  cf[ 1] = C.coefsLow[1];  cf[ 2] = C.coefsLow[2];
    // } else if ( j == 1) {
    //     cf[ 0] = C.coefsLow[1];  cf[ 1] = C.coefsLow[2];  cf[ 2] = C.coefsLow[3];
    // } else if ( j == 2) {
    //     cf[ 0] = C.coefsLow[2];  cf[ 1] = C.coefsLow[3];  cf[ 2] = C.coefsLow[4];
    // } else if ( j == 3) {
    //     cf[ 0] = C.coefsLow[3];  cf[ 1] = C.coefsLow[4];  cf[ 2] = C.coefsLow[5];
    // } else if ( j == 4) {
    //     cf[ 0] = C.coefsLow[4];  cf[ 1] = C.coefsLow[5];  cf[ 2] = C.coefsLow[6];
    // } else if ( j == 5) {
    //     cf[ 0] = C.coefsLow[5];  cf[ 1] = C.coefsLow[6];  cf[ 2] = C.coefsLow[7];
    // } else if ( j == 6) {
    //     cf[ 0] = C.coefsLow[6];  cf[ 1] = C.coefsLow[7];  cf[ 2] = C.coefsLow[8];
    // } 
    
    float3 monomials = { t * t, t, 1. };
    logy = dot( monomials, mult_f3_f33( cf, M));

  } else if (( logx >= log10(C.midPoint.x) ) && ( logx < log10(C.maxPoint.x) )) {

    float knot_coord = (N_KNOTS_HIGH-1) * (logx-log10(C.midPoint.x))/(log10(C.maxPoint.x)-log10(C.midPoint.x));
    int j = knot_coord;
    float t = knot_coord - j;

    float3 cf = { C.coefsHigh[ j], C.coefsHigh[ j + 1], C.coefsHigh[ j + 2]}; 
    // NOTE: If the running a version of CTL < 1.5, you may get an 
    // exception thrown error, usually accompanied by "Array index out of range" 
    // If you receive this error, it is recommended that you update to CTL v1.5, 
    // which contains a number of important bug fixes. Otherwise, you may try 
    // uncommenting the below, which is longer, but equivalent to, the above 
    // line of code.
    //
    //float3 cf;
    //if ( j <= 0) {
    //    cf[ 0] = C.coefsHigh[0];  cf[ 1] = C.coefsHigh[1];  cf[ 2] = C.coefsHigh[2];
    //} else if ( j == 1) {
    //    cf[ 0] = C.coefsHigh[1];  cf[ 1] = C.coefsHigh[2];  cf[ 2] = C.coefsHigh[3];
    //} else if ( j == 2) {
    //    cf[ 0] = C.coefsHigh[2];  cf[ 1] = C.coefsHigh[3];  cf[ 2] = C.coefsHigh[4];
    //} else if ( j == 3) {
    //    cf[ 0] = C.coefsHigh[3];  cf[ 1] = C.coefsHigh[4];  cf[ 2] = C.coefsHigh[5];
    //} else if ( j == 4) {
    //    cf[ 0] = C.coefsHigh[4];  cf[ 1] = C.coefsHigh[5];  cf[ 2] = C.coefsHigh[6];
    //} else if ( j == 5) {
    //    cf[ 0] = C.coefsHigh[5];  cf[ 1] = C.coefsHigh[6];  cf[ 2] = C.coefsHigh[7];
    //} else if ( j == 6) {
    //    cf[ 0] = C.coefsHigh[6];  cf[ 1] = C.coefsHigh[7];  cf[ 2] = C.coefsHigh[8];
    //} 

    float3 monomials = { t * t, t, 1. };
    logy = dot( monomials, mult_f3_f33( cf, M));
  } else { //if ( logIn >= log10(C.maxPoint.x) ) { 
    logy = logx * C.slopeHigh + ( log10(C.maxPoint.y) - C.slopeHigh * log10(C.maxPoint.x) );
  }
  return pow10(logy);
}


float segmented_spline_c9_rev(float y, SegmentedSplineParams_c9 C = ODT_48nits) {  
  static const int N_KNOTS_LOW = 8;
  static const int N_KNOTS_HIGH = 8;

  static const float KNOT_INC_LOW = (log10(C.midPoint.x) - log10(C.minPoint.x)) / (N_KNOTS_LOW - 1.);
  static const float KNOT_INC_HIGH = (log10(C.maxPoint.x) - log10(C.midPoint.x)) / (N_KNOTS_HIGH - 1.);
  
  // KNOT_Y is luminance of the spline at each knot
  float KNOT_Y_LOW[ N_KNOTS_LOW];
  for (int i = 0; i < N_KNOTS_LOW; i = i+1) {
    KNOT_Y_LOW[ i] = ( C.coefsLow[i] + C.coefsLow[i+1]) / 2.;
  };

  float KNOT_Y_HIGH[ N_KNOTS_HIGH];
  for (int k = 0; k < N_KNOTS_HIGH; k = k+1) {
    KNOT_Y_HIGH[k] = ( C.coefsHigh[k] + C.coefsHigh[k+1]) / 2.;
  };

  float logy = log10( max( y, TINY));

  float logx;
  if (logy <= log10(C.minPoint.y)) {

    logx = log10(C.minPoint.x);

  } else if ( (logy > log10(C.minPoint.y)) && (logy <= log10(C.midPoint.y)) ) {

    unsigned int j;
    float3 cf;
    if ( logy > KNOT_Y_LOW[ 0] && logy <= KNOT_Y_LOW[ 1]) {
        cf[ 0] = C.coefsLow[0];  cf[ 1] = C.coefsLow[1];  cf[ 2] = C.coefsLow[2];  j = 0;
    } else if ( logy > KNOT_Y_LOW[ 1] && logy <= KNOT_Y_LOW[ 2]) {
        cf[ 0] = C.coefsLow[1];  cf[ 1] = C.coefsLow[2];  cf[ 2] = C.coefsLow[3];  j = 1;
    } else if ( logy > KNOT_Y_LOW[ 2] && logy <= KNOT_Y_LOW[ 3]) {
        cf[ 0] = C.coefsLow[2];  cf[ 1] = C.coefsLow[3];  cf[ 2] = C.coefsLow[4];  j = 2;
    } else if ( logy > KNOT_Y_LOW[ 3] && logy <= KNOT_Y_LOW[ 4]) {
        cf[ 0] = C.coefsLow[3];  cf[ 1] = C.coefsLow[4];  cf[ 2] = C.coefsLow[5];  j = 3;
    } else if ( logy > KNOT_Y_LOW[ 4] && logy <= KNOT_Y_LOW[ 5]) {
        cf[ 0] = C.coefsLow[4];  cf[ 1] = C.coefsLow[5];  cf[ 2] = C.coefsLow[6];  j = 4;
    } else if ( logy > KNOT_Y_LOW[ 5] && logy <= KNOT_Y_LOW[ 6]) {
        cf[ 0] = C.coefsLow[5];  cf[ 1] = C.coefsLow[6];  cf[ 2] = C.coefsLow[7];  j = 5;
    } else if ( logy > KNOT_Y_LOW[ 6] && logy <= KNOT_Y_LOW[ 7]) {
        cf[ 0] = C.coefsLow[6];  cf[ 1] = C.coefsLow[7];  cf[ 2] = C.coefsLow[8];  j = 6;
    }
    
    const float3 tmp = mult_f3_f33( cf, M);

    float a = tmp[ 0];
    float b = tmp[ 1];
    float c = tmp[ 2];
    c = c - logy;

    const float d = sqrt( b * b - 4. * a * c);

    const float t = ( 2. * c) / ( -d - b);

    logx = log10(C.minPoint.x) + ( t + j) * KNOT_INC_LOW;

  } else if ( (logy > log10(C.midPoint.y)) && (logy < log10(C.maxPoint.y)) ) {

    unsigned int j;
    float3 cf;
    if ( logy > KNOT_Y_HIGH[ 0] && logy <= KNOT_Y_HIGH[ 1]) {
        cf[ 0] = C.coefsHigh[0];  cf[ 1] = C.coefsHigh[1];  cf[ 2] = C.coefsHigh[2];  j = 0;
    } else if ( logy > KNOT_Y_HIGH[ 1] && logy <= KNOT_Y_HIGH[ 2]) {
        cf[ 0] = C.coefsHigh[1];  cf[ 1] = C.coefsHigh[2];  cf[ 2] = C.coefsHigh[3];  j = 1;
    } else if ( logy > KNOT_Y_HIGH[ 2] && logy <= KNOT_Y_HIGH[ 3]) {
        cf[ 0] = C.coefsHigh[2];  cf[ 1] = C.coefsHigh[3];  cf[ 2] = C.coefsHigh[4];  j = 2;
    } else if ( logy > KNOT_Y_HIGH[ 3] && logy <= KNOT_Y_HIGH[ 4]) {
        cf[ 0] = C.coefsHigh[3];  cf[ 1] = C.coefsHigh[4];  cf[ 2] = C.coefsHigh[5];  j = 3;
    } else if ( logy > KNOT_Y_HIGH[ 4] && logy <= KNOT_Y_HIGH[ 5]) {
        cf[ 0] = C.coefsHigh[4];  cf[ 1] = C.coefsHigh[5];  cf[ 2] = C.coefsHigh[6];  j = 4;
    } else if ( logy > KNOT_Y_HIGH[ 5] && logy <= KNOT_Y_HIGH[ 6]) {
        cf[ 0] = C.coefsHigh[5];  cf[ 1] = C.coefsHigh[6];  cf[ 2] = C.coefsHigh[7];  j = 5;
    } else if ( logy > KNOT_Y_HIGH[ 6] && logy <= KNOT_Y_HIGH[ 7]) {
        cf[ 0] = C.coefsHigh[6];  cf[ 1] = C.coefsHigh[7];  cf[ 2] = C.coefsHigh[8];  j = 6;
    }
    
    const float3 tmp = mult_f3_f33( cf, M);

    float a = tmp[ 0];
    float b = tmp[ 1];
    float c = tmp[ 2];
    c = c - logy;

    const float d = sqrt( b * b - 4. * a * c);

    const float t = ( 2. * c) / ( -d - b);

    logx = log10(C.midPoint.x) + ( t + j) * KNOT_INC_HIGH;

  } else { //if ( logy >= log10(C.maxPoint.y) ) {

    logx = log10(C.maxPoint.x);

  }
  
  return pow10( logx);
}

#endif
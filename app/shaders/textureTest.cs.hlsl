#include "textureTest.if.hpp"

struct RayData
{
	float2 fragCoord;
	float3 ro;
	float3 direction;
	float3 mixColor;
	float3 lightPos;
};

groupshared RayData rdata[64];

#define MAX_ITER 140.0
#define EPSILON 0.000005
#define PII 3.14
// how many reflections
#define MAXREFLECTION 2.0
// how much object reflects light
#define REFLECTIVITY 0.3
// How far rays can travel(applies to first ray only)
#define MAXLENGTH 180.0
// color to red by iteration multiplier
#define COLORBYITER 0.0
// This multiplies the one intensity of the light
#define LIGHTINTENSITY 10.0
// this is basically resolution multiplier, one pixel is RAYMULTIPLIER*RAYMULTIPLIER amount of rays
#define RAYMULTIPLIER 1.0
// kind of deprecated, adds walls/cube to somewhere.


float3 rotateX(float3 p, float phi) {
  float c = cos(phi);
  float s = sin(phi);
  return float3(p.x, c*p.y - s*p.z, s*p.y + c*p.z);
}

float3 rotateY(float3 p, float phi) {
  float c = cos(phi);
  float s = sin(phi);
  return float3(c*p.x + s*p.z, p.y, c*p.z - s*p.x);
}

float3 rotateZ(float3 p, float phi) {
  float c = cos(phi);
  float s = sin(phi);
  return float3(c*p.x - s*p.y, s*p.x + c*p.y, p.z);
}

/////PRIMITIVES///////////////////////////////////////////////////////
// simple floor to the scene


float sFloor (float3 pos, float y){
  return abs(y-pos.y);
}

float sBall(float3 pos, float size) {
  return length(pos)-size;
  // pos 3.0, raypos 1.0, size 1.0, expected something between 1-2 result
}

float udBox( float3 p, float3 b )
{
  return length(max(abs(p)-b,0.0))-0.01;
}

float revBox( float3 p, float3 b )
{
  return length(max(abs(p)-b,0.0));
}

float rotBox( float3 p, float3 b )
{
  return udBox(rotateZ(rotateY(p, sin(iTime*0.1)*PII), cos(iTime*0.1)*PII), b);
}

float sdHexPrism( float3 p, float2 h )
{
    float3 q = abs(p);
    return max(q.z-h.y,max(q.x+q.y*0.57735,q.y*1.1547)-h.x);
}

float opRepBox( float3 p, float3 c )
{
    float3 q = p%c-0.5*c;
    return udBox( q , float3(0.15, 0.15, 0.15) );
}

float opRepPrism( float3 p, float3 c )
{
    float3 q = p%c-0.5*c;
    return sdHexPrism( rotateX(q,q.x*2.0) , float2(0.15, 0.15) );
}

float opS( float f1, float f2 )
{
  return (f1<f2) ? f1 : f2;
}

float2 opU( float2 d1, float2 d2 )
{
  return (d1.x<d2.x) ? d1 : d2;
}

float2 map2(float3 pos)
{
  //float box = rotBox(pos-float3(1.0,sin(iTime*0.3)*0.6,.6), float3(0.3, 0.3, 0.3));
  //float ball = sBall(pos-float3(-.2,0.1,.5),  0.4);
  //float ball2 = sBall(pos-float3(-.2,0.1,-.5),  0.4);
  //float3 rot = rotateX(pos-float3(-.2,-.5,.0),sin(iTime*0.2)*PII);
  //float prism = sdHexPrism(rot,float2(0.2, 0.2));
  float what = opRepBox( pos-float3(-4999.3,0.2,0.0), float3(1.0,0.6, .6));
  float2 res = float2(what,4.0);
  //float2 res = float2(box,2.0);
  //res = opU(res, float2(ball,2.0));
  //res = opU(res, float2(ball2,1.0));
  //res = opU(res, float2(prism,4.0));
  //float what2 = opRepBox( pos - float3(1.0f. 1.0f, 1.0f), float3(0.5f, 0.5f, 0.5f));
  //res = opU(res, float2(what2,2.0));
//walls
  //res = opU(res, sBall(pos-float3(0.2f,0.2f,1.5f), 0.4f));
  //res = opU(res, rotBox(pos-float3(0.0f,0.0f,2.5f), float3(0.2f, 0.2f 0.2f)));
  return res;
}

float3 getNor2(float3 pos)
{
  float2 e = float2(EPSILON*100.0, 0.0);
  float3 nor = float3(
      map2(pos+e.xyy).x - map2(pos-e.xyy).x,
      map2(pos+e.yxy).x - map2(pos-e.yxy).x,
      map2(pos+e.yyx).x - map2(pos-e.yyx).x );
  return normalize(nor);
}

// this is probably a ball... I hope

//Specify all the objects that exist


float3 castRay2(float3 rp, float3 rd)
{
  const float3 ro = rp;
  float2 dist = float2(100000000.0, 100000000.0);
  float it=0.0;
  for (float i = 0.0; i<MAX_ITER; i++) {
    it++;
    dist = map2(rp);
    if (dist.x < EPSILON) break;
    rp = rp+rd*dist.x*0.9;
  }
  return float3(distance(ro,rp), it, dist.y);
}


float3 shadowray2(float3 hitSpot, float3 lightPos) {
  const float3 rd = normalize(lightPos-hitSpot);
  float3 rp = hitSpot+rd*0.001;

  float2 dist = float2(100000000.0, 100000000.0);
  for (float i = 0.0; i<MAX_ITER; i++)
  {
    dist = map2(rp);
    dist.x = min(dist.x, distance(rp,lightPos));
    if (dist.x < EPSILON) break;
    rp = rp+rd*dist.x*0.9;
  }
  if (distance(rp,lightPos) > EPSILON)
    return float3(0.1, 0.1, 0.1);
  
  const float3 normal = getNor2(hitSpot);
  float3 AOL = float3(1.0,1.0,1.0)*0.1;

  const float d = abs(dot(rd,normal));
  const float lenToLight = distance(hitSpot, lightPos);
  if (d > 0.0001)
  {
    AOL = pow(d*LIGHTINTENSITY,2.0)/lenToLight+float3(1.0,1.0,1.0)*0.1;
  }
  return AOL;
}

float3 getCameraRay(float3 posi, float3 dir, float2 pos) {
  float3 image_point = pos.x * iSideDir + pos.y*iUpDir + posi + dir;
  return normalize(image_point-posi);
}

float3 getColor(float id)
{
  if (id == 1.0)
    return float3(1.0,float2(0.0, 0.0));
  else if (id == 2.0)
    return float3(0.0,1.0,0.0);
  else if (id == 3.0)
    return float3(0.0,0.0,1.0);
  return float3 (0.2,0.2,0.2);
}

//float3 calcRay(float3 ro, float3 rd, float3 lightPos)
float3 calcRay(in uint index,in float3 ro, in float3 rd)
{
  float lightMultiplier = 1.0f;
  float3 res = castRay2(ro, rd);
  float iter = res.y;
  float3 aol2 = float3(0.0, 0.0,0.0);
  float3 color = float3(0.0, 0.0, 0.0);
  if (res.y < MAX_ITER && res.x < MAXLENGTH)
  {
    for (float i = 0.0;i<MAXREFLECTION;i++)
    {
      // castray found object
      float3 pnt = ro+rd*res.x; // hitted point
      float3 pointNormal = getNor2(pnt); // surface normal
      float3 reflectDir = rd - 2.0*pointNormal*dot(rd,pointNormal);
      float3 aol = shadowray2(pnt,rdata[index].lightPos); // get the light
      aol2 += aol*getColor(res.z)*lightMultiplier * REFLECTIVITY;
      lightMultiplier = lightMultiplier * REFLECTIVITY;
      res = castRay2(pnt+reflectDir*0.001, reflectDir);
      ro = pnt;
      rd = reflectDir;
    }
    color += (aol2 + COLORBYITER*0.3*float3(iter/MAX_ITER,.0*iter/MAX_ITER,.0*iter/MAX_ITER));
  }
  return color;
}

float2 getHigherResolutionPos(in float2 fragCoord, float x, float y)
{
  float2 biggerRes = iResolution.xy*RAYMULTIPLIER;
  float2 screenPos = fragCoord.xy*RAYMULTIPLIER; //oletus, aina pikseli blokin ylÃ¤vasen
  screenPos.x += x;
  screenPos.y += y;

  float aspect = biggerRes.x/biggerRes.y;
  float2 uvUnit = 1.0 / biggerRes.xy;
  float2 uv = ( screenPos / biggerRes.xy );
  float2 pos = (uv-0.5);
  pos.x *= aspect;
  pos.y -= 0.0;
  return pos;
}

//float3 castManyRays(in float2 fragCoord,in float3 ro, in float3 direction)
void castManyRays(in uint index)
{
  const float rayMulti = RAYMULTIPLIER - 1.0;
  float2 screenPos = rdata[index].fragCoord.xy;
  const float epsilon = 0.001;
  const float colorMultiplier = 1.0 / (pow(rayMulti,2.0) + 2.0*rayMulti + 1.0);
  //float3 mixColor = float3(0,0,0);
  const float3 camDir = normalize(rdata[index].direction);
  for (float x = 0.0; x <= RAYMULTIPLIER-1.0; x++)
  {
	  for (float y = 0.0; y <= RAYMULTIPLIER-1.0; y++)
	  {
		  float3 rd = getCameraRay(rdata[index].ro, camDir, getHigherResolutionPos(rdata[index].fragCoord, x,y));
		  rdata[index].mixColor += colorMultiplier * calcRay(index,rdata[index].ro, rd);
		  screenPos.y+=epsilon;
	  }
	  screenPos.x+=epsilon;
  }
}

// The code in this file was originally written by Stephen Hill (@self_shadow), who deserves all
// credit for coming up with this fit and implementing it. Buy him a beer next time you see him. :)

// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
static const float3x3 ACESInputMat =
{
    { 0.59719, 0.35458, 0.04823 },
    { 0.07600, 0.90834, 0.01566 },
    { 0.02840, 0.13383, 0.83777 }
};

// ODT_SAT => XYZ => D60_2_D65 => sRGB
static const float3x3 ACESOutputMat =
{
    { 1.60475, -0.53108, -0.07367 },
    { -0.10208,  1.10813, -0.00605 },
    { -0.00327, -0.07276,  1.07602 }
};

float3 RRTAndODTFit(float3 v)
{
    float3 a = v * (v + 0.0245786f) - 0.000090537f;
    float3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

float3 ACESFitted(float3 color)
{
    color = mul(ACESInputMat, color);

    // Apply RRT and ODT
    color = RRTAndODTFit(color);

    color = mul(ACESOutputMat, color);

    // Clamp to [0, 1]
    color = saturate(color);

    return color;
}

CS_SIGNATURE
void main(uint2 id : SV_DispatchThreadID, uint2 gid : SV_GroupThreadID)
{

	
	float2 fp = float2(float(id.x), float(id.y));
	//fp /= float2(float(iResolution.x), float(iResolution.y));

	//output[id] = float4(sin(iTime)*0.2 + fp.x, cos(iTime)*0.5 + fp.y, 0.f, 1.f);

	float3 position = iPos;
  float3 direction = iDir;

  direction = normalize(direction);
  uint index = gid.x + gid.y*8;

  rdata[index].fragCoord = fp;
  rdata[index].ro = position;
  //rdata[index].ro = float3(0.f, 0.f, 0.f);
	//rdata[index].direction = direction;
  rdata[index].direction = direction;
  //rdata[index].direction.x = sin(iTime);
  //rdata[index].direction.z = cos(iTime);
	rdata[index].mixColor = float3(0.f, 0.f, 0.f);
	rdata[index].lightPos = float3(-3.4 + sin(iTime*0.3)*0.2,3.0,-1.5);
	//GroupMemoryBarrierWithGroupSync();
  castManyRays(index);
  if (id.x >= iResolution.x || id.y >= iResolution.y)
		return;
  output[id] = float4(ACESFitted(rdata[index].mixColor),1.0);
}
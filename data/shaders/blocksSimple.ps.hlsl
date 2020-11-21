#include "blocksSimple.if.hlsl"
#include "lib/color.hlsl"
// this is trying to be Pixel shader file.
struct VertexOut {
  float2 uv : TEXCOORD0;
  float4 wPos : VIEWSPACEPOS; 
  float4 pos : SV_Position;
};

struct PixelOut
{
  float4 color: SV_Target0;
  float4 motion: SV_Target1;
};

float3 HUEtoRGB(in float H)
  {
    float R = abs(H * 6 - 3) - 1;
    float G = 2 - abs(H * 6 - 2);
    float B = 2 - abs(H * 6 - 4);
    return saturate(float3(R,G,B));
  }

[RootSignature(ROOTSIG)]
PixelOut main(VertexOut input)
{
  PixelOut output;
  output.color = float4(input.uv, 0, 1);

  uint val = constants.cubeMaterial % 63;

  float3 color = HUEtoRGB(val/63.f);
  output.color.rgb = color/2.f;
  output.color.rg += input.uv;
  //output.color.rg = input.uv;

  // motion
  CameraSettings previousCamera = cameras.Load(constants.camera.previous);
  float4 prevPos = mul(float4(input.wPos.xyz,1), previousCamera.perspective);
  float4 curPos = input.pos;
  curPos.xy /=  constants.outputSize;
  curPos.xy -= float2(0.5, 0.5);
  curPos.xy *= float2(2,-2);
  curPos.w = 1.f;
  prevPos = prevPos / prevPos.w;
  output.motion = prevPos - curPos; 
  output.motion.xy *= float2(1, -1);

  return output;
}

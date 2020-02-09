#include "testMeshShaders.if.hlsl"
#include "testMesh_payload.hlsl"

[RootSignature(ROOTSIG)]
float4 main(VertexOut input) : SV_TARGET
{
  float3 hsv = float3((input.id * 4) / float(constants.meshletCount), 1, 1);
  float3 color = input.normal;
  color = hsvToRgb(hsv);
  return float4(color,1);
}

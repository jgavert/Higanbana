#include "testMeshShaders.if.hlsl"

struct VertexOut
{
  float2 uv : TEXCOORD0;
  float3 normal : NORMAL;
  float4 pos : SV_Position;
};

struct payloadStruct
{ 
    uint myArbitraryData; 
}; 

[RootSignature(ROOTSIG)]
[NumThreads(32, 1, 1)] 
[OutputTopology("triangle")] 
void main( 
    uint gtid : SV_GroupThreadID, 
    uint gid : SV_GroupID, 
    in payload payloadStruct MeshPayload,
    out indices uint3 tris[1], 
    out vertices VertexOut verts[3] 
) 
{
    SetMeshOutputCounts(3, 1); 
    if (gtid < 1) 
    { 
        tris[gtid] = uint3(0,0,0); 
    } 
 
    if (gtid < 3) 
    {
        VertexOut vout;
        vout.uv = float2(0,0);
        vout.normal = float3(0,0,0);
        vout.pos = float4(0,0,0,1);
        verts[gtid] = vout;
    } 
}
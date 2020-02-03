#include "testMeshShaders.if.hlsl"

struct VertexOut
{
  float2 uv : TEXCOORD0;
  float3 normal : NORMAL;
  float4 pos : SV_Position;
};

struct payloadStruct
{ 
  float4x4 perspective; 
}; 

[RootSignature(ROOTSIG)]
[NumThreads(32, 1, 1)] 
[OutputTopology("triangle")] 
void main( 
  uint gtid : SV_GroupThreadID, 
  uint gid : SV_GroupID, 
  in payload payloadStruct MeshPayload,
  out indices uint3 tris[12], 
  out vertices VertexOut verts[32] 
) 
{
  WorldMeshlet m = meshlets[gid];
  SetMeshOutputCounts(m.vertices, m.primitives); 
  if (gtid < m.primitives) 
  { 
    uint offset = gtid * 3;
    uint i0 = packedIndices.Load(m.offsetPacked + offset);
    uint i1 = packedIndices.Load(m.offsetPacked + offset + 1);
    uint i2 = packedIndices.Load(m.offsetPacked + offset + 2);
    tris[gtid] = uint3(i0, i1, i2);
  } 

  if (gtid < m.vertices) 
  {
    uint index = uniqueIndices.Load(m.offsetUnique + gtid);

    float3 vert = vertices.Load(index);

    VertexOut vout;
    vout.uv = uvs.Load(index);
    vout.normal = normals.Load(index);
    vout.pos = mul(float4(vert,1), MeshPayload.perspective);

    verts[gtid] = vout;
  } 
}
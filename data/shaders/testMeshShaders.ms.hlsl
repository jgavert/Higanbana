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
  out indices uint3 tris[126], 
  out vertices VertexOut verts[64] 
) 
{
  WorldMeshlet m = meshlets[gid];
  SetMeshOutputCounts(m.vertices, m.primitives); 
  for (int i = 0; i < 2; ++i)
  {
    uint ti = gtid * 2 + i;
    uint index = uniqueIndices.Load(m.offsetUnique + ti);

    float3 vert = vertices.Load(index);

    VertexOut vout;
    vout.uv = uvs.Load(index);
    vout.normal = normals.Load(index);
    vout.pos = mul(float4(vert,1), MeshPayload.perspective);

    verts[ti] = vout;
  }

  for (int k = 0; k < 4; ++k)
  {
    uint ti = gtid * 4 + k;
    uint offset = ti * 3;
    uint i0 = packedIndices.Load(m.offsetPacked + offset);
    uint i1 = packedIndices.Load(m.offsetPacked + offset + 1);
    uint i2 = packedIndices.Load(m.offsetPacked + offset + 2);
    tris[ti] = uint3(i0, i1, i2);
  }
}
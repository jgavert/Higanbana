#include "testMeshShaders.if.hlsl"
#include "testMesh_payload.hlsl"

struct VertexOut
{
  uint id : COLOR0;
  float2 uv : TEXCOORD0;
  float3 normal : NORMAL;
  float4 pos : SV_Position;
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
  uint meshletId = gid;
  WorldMeshlet m = meshlets[meshletId];
  SetMeshOutputCounts(m.vertices, m.primitives); 
  for (int i = 0; i < 2; ++i)
  {
    uint ti = gtid + i * 32;
    uint index = uniqueIndices.Load(m.offsetUnique + ti);

    float3 vert = vertices.Load(index);

    VertexOut vout;
    vout.id = meshletId;
    vout.uv = uvs.Load(index);
    vout.normal = normals.Load(index);
    vout.pos = mul(float4(vert,1), MeshPayload.perspective);

    verts[ti] = vout;
  }

  for (int k = 0; k < 4; ++k)
  {
    uint ti = gtid + k * 32;
    uint offset = ti * 3;
    uint i0 = packedIndices.Load(m.offsetPacked + offset);
    uint i1 = packedIndices.Load(m.offsetPacked + offset + 1);
    uint i2 = packedIndices.Load(m.offsetPacked + offset + 2);
    tris[ti] = uint3(i0, i1, i2);
  }
}
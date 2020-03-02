#include "meshes.hpp"
#include "mesh_test.hpp"

struct MeshletPrepare
{
  higanbana::vector<uint32_t> uniqueIndexes;
  higanbana::vector<uint8_t> packedIndexes;
  higanbana::vector<WorldMeshlet> meshlets;
};

namespace app
{
template <typename T>
MeshletPrepare createMeshlets(const higanbana::MemView<T>& indices) {
  MeshletPrepare meshlets{};
  constexpr const int maxUniqueVertices = 64; // max indice count per meshlet
  constexpr const int maxPrimitives = 126; // max indice count per meshlet
  size_t i = 0;
  size_t max_size = indices.size();
  WorldMeshlet currentMeshlet{};
  while (i < max_size) {
    if (currentMeshlet.primitives + 1 > maxPrimitives || currentMeshlet.vertices + 3 > maxUniqueVertices)
    {
      meshlets.meshlets.push_back(currentMeshlet);
      currentMeshlet = WorldMeshlet{0, 0, static_cast<uint>(meshlets.uniqueIndexes.size()), static_cast<uint>(meshlets.packedIndexes.size())};
    }
    uint i1 = indices[i];
    uint i2 = indices[i+1];
    uint i3 = indices[i+2];
    i = i+3;
    currentMeshlet.primitives++;

    auto getPackedIndex = [](higanbana::vector<uint32_t>& ui, uint& vertices, uint uOffset, uint value) -> uint8_t {
      for (size_t i = uOffset; i < ui.size(); i++)
      {
        if (ui[i] == value)
          return i - uOffset;
      }
      size_t packedIndex = ui.size() - uOffset;
      ui.push_back(value);
      vertices++;
      HIGAN_ASSERT(packedIndex < 256, "uh oh");
      return static_cast<uint8_t>(packedIndex);
    };
    auto v = getPackedIndex(meshlets.uniqueIndexes, currentMeshlet.vertices, currentMeshlet.offsetUnique, i1);
    meshlets.packedIndexes.push_back(v);
    v = getPackedIndex(meshlets.uniqueIndexes, currentMeshlet.vertices, currentMeshlet.offsetUnique, i2);
    meshlets.packedIndexes.push_back(v);
    v = getPackedIndex(meshlets.uniqueIndexes, currentMeshlet.vertices, currentMeshlet.offsetUnique, i3);
    meshlets.packedIndexes.push_back(v);
  }
  meshlets.meshlets.push_back(currentMeshlet);

  return meshlets;
}

int MeshSystem::allocate(higanbana::GpuGroup& gpu, higanbana::ShaderArgumentsLayout& normalLayout,higanbana::ShaderArgumentsLayout& meshLayout, MeshData& data, int buffers[5]) {
  using namespace higanbana;
  auto val = freelist.allocate();
  if (views.size() < val+1) views.resize(val+1);

  auto sizeInfo = higanbana::formatSizeInfo(data.indices.format);
  auto& view = views[val];
  ShaderViewDescriptor svd = ShaderViewDescriptor()
    .setFormat(data.indices.format)
    .setFirstElement(sourceBuffers[buffers[0]].offset)
    .setElementCount(data.indices.size / sizeInfo.pixelSize);
  view.indices = gpu.createBufferIBV(meshbuffer, svd);
  // mesh shader required ... data is on gpu so old pipe is useless
  /*

  MeshletPrepare meshlets;
  if (data.indiceFormat == FormatType::Uint16)
  {
    meshlets = createMeshlets(makeMemView(reinterpret_cast<uint16_t*>(data.indices.data()), data.indices.size()/2));
  }
  else
  {
    meshlets = createMeshlets(makeMemView(reinterpret_cast<uint32_t*>(data.indices.data()), data.indices.size()/4));
  }

  view.uniqueIndices = gpu.createBufferSRV(ResourceDescriptor()
    .setName("uniqueIndices")
    .setFormat(FormatType::Uint32)
    .setCount(meshlets.uniqueIndexes.size())
    .setUsage(ResourceUsage::GpuReadOnly));
  view.packedIndices = gpu.createBufferSRV(ResourceDescriptor()
    .setName("packedIndices")
    .setFormat(FormatType::Uint8)
    .setCount(meshlets.packedIndexes.size())
    .setUsage(ResourceUsage::GpuReadOnly));
  view.meshlets = gpu.createBufferSRV(ResourceDescriptor()
    .setName("meshlets")
    .setCount(meshlets.meshlets.size())
    .setUsage(ResourceUsage::GpuReadOnly)
    .setStructured<WorldMeshlet>());
  // end mesh shader things
  */

  sizeInfo = higanbana::formatSizeInfo(data.vertices.format);
  svd = ShaderViewDescriptor()
    .setFormat(data.vertices.format)
    .setFirstElement(sourceBuffers[buffers[1]].offset)
    .setElementCount(data.vertices.size / sizeInfo.pixelSize);
  view.vertices = gpu.createBufferSRV(meshbuffer, svd);

  if (data.texCoords.format != FormatType::Unknown)
  {
    sizeInfo = higanbana::formatSizeInfo(data.texCoords.format);
    svd = ShaderViewDescriptor()
      .setFormat(data.texCoords.format)
      .setFirstElement(sourceBuffers[buffers[3]].offset)
      .setElementCount(data.texCoords.size / sizeInfo.pixelSize);
    view.uvs = gpu.createBufferSRV(meshbuffer, svd);
  }

  if (data.normals.format != FormatType::Unknown)
  {
    sizeInfo = higanbana::formatSizeInfo(data.normals.format);
    svd = ShaderViewDescriptor()
      .setFormat(data.normals.format)
      .setFirstElement(sourceBuffers[buffers[2]].offset)
      .setElementCount(data.normals.size / sizeInfo.pixelSize);
    view.normals = gpu.createBufferSRV(meshbuffer, svd);
  }

  view.args = gpu.createShaderArguments(higanbana::ShaderArgumentsDescriptor("World shader layout", normalLayout)
    .bind("vertices", view.vertices)
    .bind("uvs", view.uvs)
    .bind("normals", view.normals));

  view.meshArgs = gpu.createShaderArguments(higanbana::ShaderArgumentsDescriptor("World shader layout(mesh version)", meshLayout)
    .bind("meshlets", view.meshlets)
    .bind("uniqueIndices", view.uniqueIndices)
    .bind("packedIndices", view.packedIndices)
    .bind("vertices", view.vertices)
    .bind("uvs", view.uvs)
    .bind("normals", view.normals));

  /*
  auto graph = gpu.createGraph();
  auto node = graph.createPass("Update mesh data");
  auto indData = gpu.dynamicBuffer(makeMemView(data.indices), data.indices.format);
  node.copy(view.indices.buffer(), indData);
  auto vertData = gpu.dynamicBuffer(makeMemView(data.vertices), data.vertices.format);
  node.copy(view.vertices.buffer(), vertData);
  if (data.texCoords.format != FormatType::Unknown)
  {
    auto uvData = gpu.dynamicBuffer(makeMemView(data.texCoords), data.texCoords.format); 
    node.copy(view.uvs.buffer(), uvData);
  }
  if (data.normals.format != FormatType::Unknown)
  {
    auto normalData = gpu.dynamicBuffer(makeMemView(data.normals), data.normals.format); 
    node.copy(view.normals.buffer(), normalData);
  }

  {
    auto uniqueIndex = gpu.dynamicBuffer(makeMemView(meshlets.uniqueIndexes), FormatType::Uint32);
    node.copy(view.uniqueIndices.buffer(), uniqueIndex);
    auto packedData = gpu.dynamicBuffer(makeMemView(meshlets.packedIndexes), FormatType::Uint8);
    node.copy(view.packedIndices.buffer(), packedData);
    auto meshletData = gpu.dynamicBuffer(makeMemView(meshlets.meshlets));
    node.copy(view.meshlets.buffer(), meshletData);
  }

  graph.addPass(std::move(node));
  gpu.submit(graph);
  */
  return val;
}

void MeshSystem::free(int index)
{
  views[index] = {};
  freelist.release(index);
}
int MeshSystem::allocateBuffer(higanbana::GpuGroup& gpu, BufferData& data) {
  using namespace higanbana;
  auto val = freelistBuffers.allocate();
  if (sourceBuffers.size() < val+1) sourceBuffers.resize(val+1);

  auto freeSpace = meshbufferAllocator.allocate(data.data.size());
  Buffer updateTarget;
  if (!freeSpace)
  {
    meshbufferAllocator.resize(meshbufferAllocator.size() + data.data.size());
    updateTarget = gpu.createBuffer(ResourceDescriptor()
      .setName("meshbuffer")
      .setCount(data.data.size())
      .setFormat(FormatType::Unorm8)
      .setUsage(ResourceUsage::GpuReadOnly)
      .setIndexBuffer());
    freeSpace = meshbufferAllocator.allocate(data.data.size());
  }
  sourceBuffers[val] = freeSpace.value();

  auto graph = gpu.createGraph();
  auto node = graph.createPass("Update buffer data");
  if (!updateTarget.desc().desc.name.empty())
  {
    node.copy(updateTarget, meshbuffer);
    meshbuffer = updateTarget;
  }
  auto dynamic = gpu.dynamicBuffer(makeMemView(data.data), FormatType::Unorm8);
  node.copy(meshbuffer, freeSpace.value().offset, dynamic);
  return val;
}
void MeshSystem::freeBuffer(int index) {
  meshbufferAllocator.free(sourceBuffers[index]);
  sourceBuffers[index] = {};
  freelistBuffers.release(index);
}
}
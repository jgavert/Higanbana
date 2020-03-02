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
  int buffer = buffers[0];
  auto block = sourceBuffers[buffer];
  ShaderViewDescriptor svd = ShaderViewDescriptor()
    .setFormat(data.indices.format)
    .setFirstElement(block.offset / sizeInfo.pixelSize)
    .setElementCount(data.indices.size / sizeInfo.pixelSize);
  HIGAN_ASSERT(block.offset % sizeInfo.pixelSize == 0, "");
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
  buffer = buffers[1];
  block = sourceBuffers[buffer];
  svd = ShaderViewDescriptor()
    .setFormat(data.vertices.format)
    .setFirstElement(block.offset / sizeInfo.pixelSize)
    .setElementCount(data.vertices.size / sizeInfo.pixelSize);
  HIGAN_ASSERT(block.offset % sizeInfo.pixelSize == 0, "");
  view.vertices = gpu.createBufferSRV(meshbuffer, svd);

  if (data.texCoords.format != FormatType::Unknown)
  {
    sizeInfo = higanbana::formatSizeInfo(data.texCoords.format);
    buffer = buffers[3];
    block = sourceBuffers[buffer];
    svd = ShaderViewDescriptor()
      .setFormat(data.texCoords.format)
      .setFirstElement(block.offset / sizeInfo.pixelSize)
      .setElementCount(data.texCoords.size / sizeInfo.pixelSize);
    HIGAN_ASSERT(block.offset % sizeInfo.pixelSize == 0, "");
    view.uvs = gpu.createBufferSRV(meshbuffer, svd);
  }

  if (data.normals.format != FormatType::Unknown)
  {
    sizeInfo = higanbana::formatSizeInfo(data.normals.format);
    buffer = buffers[2];
    block = sourceBuffers[buffer];
    svd = ShaderViewDescriptor()
      .setFormat(data.normals.format)
      .setFirstElement(block.offset / sizeInfo.pixelSize)
      .setElementCount(data.normals.size / sizeInfo.pixelSize);
    HIGAN_ASSERT(block.offset % sizeInfo.pixelSize == 0, "");
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
  if (data.data.empty())
    return -1;
  using namespace higanbana;
  auto copy = sourceBuffers;
  auto val = freelistBuffers.allocate();
  if (sourceBuffers.size() < val+1) sourceBuffers.resize(val+1);
  constexpr const int alignment = 96;
  auto freeSpace = meshbufferAllocator.allocate(data.data.size(), alignment);
  Buffer updateTarget;
  vector<RangeBlock> migrateOldBuffers;
  if (!freeSpace || freeSpace.value().size < data.data.size())
  {
    meshbufferAllocator = HeapAllocator(meshbufferAllocator.max_size() + data.data.size() + alignment);
    migrateOldBuffers.resize(copy.size());
    int index = 0;
    for (auto&& it : copy)
    {
      auto all = meshbufferAllocator.allocate(it.size, alignment);
      HIGAN_ASSERT(all, "must be true");
      migrateOldBuffers[index] = all.value();
      index++;
    }
    //meshbufferAllocator.resize(meshbufferAllocator.size() + data.data.size());
    updateTarget = gpu.createBuffer(ResourceDescriptor()
      .setName("meshbuffer")
      .setCount(meshbufferAllocator.max_size())
      .setFormat(FormatType::Unorm8)
      .setUsage(ResourceUsage::GpuReadOnly)
      .setIndexBuffer());
    freeSpace = meshbufferAllocator.allocate(data.data.size(), alignment);
  }

  auto graph = gpu.createGraph();
  auto node = graph.createPass("Update buffer data");

  if (updateTarget.handle().id != ResourceHandle::InvalidId)
  {
    if (meshbuffer.handle().id != ResourceHandle::InvalidId)
    {
      for (size_t i = 0; i < migrateOldBuffers.size(); ++i) {
        auto old = copy[i];
        auto ne = migrateOldBuffers[i];
        node.copy(updateTarget, ne.offset, meshbuffer, old.offset, old.size);
        sourceBuffers[i] = migrateOldBuffers[i];
      }
    }
    meshbuffer = updateTarget;
  }
  auto dynamic = gpu.dynamicBuffer(makeMemView(data.data), FormatType::Unorm8);
  node.copy(meshbuffer, freeSpace.value().offset, dynamic);
  graph.addPass(std::move(node));
  gpu.submit(graph);
  gpu.waitGpuIdle();
  sourceBuffers[val] = freeSpace.value();

  return val;
}
void MeshSystem::freeBuffer(int index) {
  meshbufferAllocator.free(sourceBuffers[index]);
  sourceBuffers[index] = {};
  freelistBuffers.release(index);
}
}
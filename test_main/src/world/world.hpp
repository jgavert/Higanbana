#pragma once
#include "components.hpp"
#include "visual_data_structures.hpp"

#include <higanbana/core/entity/database.hpp>
#include <higanbana/core/filesystem/filesystem.hpp>

#include <higanbana/core/datastructures/proxy.hpp>
#include <higanbana/graphics/desc/formats.hpp>
#include <higanbana/core/system/FreelistAllocator.hpp>

#include <css/task.hpp>

namespace app
{
class World 
{
  higanbana::vector<higanbana::Id> entities;
  higanbana::FreelistAllocator freelistMesh;
  higanbana::vector<MeshData> rawMeshData;
  higanbana::FreelistAllocator freelistBuffer;
  higanbana::vector<BufferData> rawBufferData;
  higanbana::FreelistAllocator freelistTexture;
  higanbana::vector<TextureData> rawTextureData;
  public:
  css::Task<void> loadGLTFSceneCgltfTasked(higanbana::Database<2048>& database, higanbana::FileSystem& fs, std::string path);
  void loadGLTFScene(higanbana::Database<2048>& database, higanbana::FileSystem& fs, std::string path);
  void loadGLTFSceneCgltf(higanbana::Database<2048>& database, higanbana::FileSystem& fs, std::string path);
  MeshData& getMesh(int index); 
  BufferData& getBuffer(int index); 
  TextureData& getTexture(int index); 
};
};
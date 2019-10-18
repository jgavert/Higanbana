#pragma once
#include "components.hpp"
#include "visual_data_structures.hpp"

#include <higanbana/core/entity/database.hpp>
#include <higanbana/core/filesystem/filesystem.hpp>

#include <higanbana/core/datastructures/proxy.hpp>
#include <higanbana/graphics/desc/formats.hpp>
#include <higanbana/core/system/FreelistAllocator.hpp>

namespace app
{
class World 
{
  higanbana::vector<higanbana::Id> entities;
  higanbana::FreelistAllocator freelist;
  higanbana::vector<MeshData> rawMeshData;
  public:
  void loadGLTFScene(higanbana::Database<2048>& database, higanbana::FileSystem& fs, std::string path);
  MeshData& getMesh(int index); 
};
};
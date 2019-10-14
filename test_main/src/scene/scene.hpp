#pragma once
#include "scene/components.hpp"

#include <higanbana/core/entity/database.hpp>
#include <higanbana/core/filesystem/filesystem.hpp>

#include <higanbana/core/datastructures/proxy.hpp>

namespace app
{
class Scene
{
  higanbana::vector<higanbana::Id> entities;
  public:
  void loadGLTFScene(higanbana::Database<2048>& database, higanbana::FileSystem& fs, std::string path);
};
};
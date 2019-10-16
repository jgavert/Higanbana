#pragma once

#include <higanbana/core/math/math.hpp>
#include <higanbana/core/datastructures/proxy.hpp>
#include <higanbana/core/entity/database.hpp>

#include <string>

namespace components
{
struct WorldPosition
{
  float3 pos;
};

struct Rotation
{
  quaternion rot;
};

struct Name
{
  std::string str;
};

struct Childs
{
  higanbana::vector<higanbana::Id> childs;
};

struct RawMeshData
{
  int id;
};
}
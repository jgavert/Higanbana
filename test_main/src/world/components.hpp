#pragma once

#include <higanbana/core/math/math.hpp>
#include <higanbana/core/datastructures/proxy.hpp>
#include <higanbana/core/entity/database.hpp>

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

struct Childs
{
  higanbana::vector<higanbana::Id> childs;
};

struct RawMeshData
{
  int id;
};
}
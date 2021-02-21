
#pragma once
#include "components.hpp"
#include "visual_data_structures.hpp"

#include <higanbana/core/entity/database.hpp>
#include <higanbana/core/filesystem/filesystem.hpp>

#include <higanbana/core/datastructures/vector.hpp>
#include <higanbana/graphics/desc/formats.hpp>
#include <higanbana/core/system/FreelistAllocator.hpp>

#include <css/task.hpp>
#include <optional>

namespace app
{

template<typename OctreeChild>
class Octree
{
  int3 origin;
  int3 halfDimension;
  std::unique_ptr<Octree> children[8];
  std::unique_ptr<OctreeChild> leaf;

  int octantContainingPoint(const int3& point) {
    int res = 0;
    res |= 4*(point.x >= origin.x);
    res |= 2*(point.y >= origin.y);
    res |= 1*(point.z >= origin.z);
    return res;
  }
  int3 indexIntoDir(int i) {
    return int3((i&4) >> 2, (i&2) >> 1, i & 1);
  }
  bool leafNode() {
    return children[0].get() == nullptr;
  }
public:
  Octree(int3 pos, int3 halfSize) : origin(pos), halfDimension(halfSize) { HIGAN_ASSERT(halfSize&1 == 0, "need to be divisible by 2"); }

  bool insertChild(int3 pos, int size, OctreeChild child) {
    if (leafNode() && halfDimension < size) {
      leaf = std::make_unique(child);
      return true;
    } else {
      // create children
      for (int i = 0; i < 8; i++) {
        children = std::make_unique<Octree>(origin + indexIntoDir(i), halfDimension / 2);
      }
      auto next = octantContainingPoint(pos);
      return children[next]->insertChild(pos, size, child);
    }
    return false; // could add or had already node
  }

  std::optional<OctreeChild> removeChild(int3 pos) {
    return std::optional<OctreeChild>();
  }

  std::optional<OctreeChild> traceRay(float3 pos, float3 dir) {
    // figure out where we are in tree
    // start tracing from there
    return std::optional<OctreeChild>();
  }
};

class ChunkWorld 
{

};
}
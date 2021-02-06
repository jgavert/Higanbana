#pragma once

#include "hittable_list.hpp"

namespace rt
{
class World
{
public:

  World();
  rt::HittableList world;
  bool worldChanged = true;
};
}
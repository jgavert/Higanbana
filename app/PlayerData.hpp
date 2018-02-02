#pragma once

#include "core/src/math/math.hpp"

namespace faze
{
  namespace fighter
  {
    class Player
    {
    public:
      double2 position;
      double2 direction;
      double2 velocity;
    };
  }
}
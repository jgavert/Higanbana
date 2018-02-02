#pragma once

#include "core/src/math/math.hpp"

namespace faze
{
  struct ColoredRect
  {
    float2 topleft;
    float2 rightBottom;
    float3 color;
  };
}
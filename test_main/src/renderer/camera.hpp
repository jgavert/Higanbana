#pragma once

#include <higanbana/core/math/math.hpp>

struct CameraSettings
{
  float fov;
  float3 pos;
  quaternion rot;
  float minZ;
  float maxZ;
  bool inverseZ;
}
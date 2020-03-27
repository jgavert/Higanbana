#pragma once

#include <higanbana/graphics/GraphicsCore.hpp>
#include <higanbana/core/math/math.hpp>

SHADER_STRUCT(CameraSettings,
  float4x4 perspective;
  float4 position;
);
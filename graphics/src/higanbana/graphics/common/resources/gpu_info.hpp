#pragma once

#include "higanbana/graphics/common/resources/graphics_api.hpp"
#include <string>

namespace higanbana
{
struct GpuInfo
{
  int id = -1;
  std::string name = "";
  int64_t memory = -1;
  VendorID vendor = VendorID::Unknown;
  uint32_t deviceId = 0; // used to match 2 devices together for interopt
  DeviceType type = DeviceType::Unknown;
  bool gpuConstants = false;
  bool canPresent = false;
  bool canRaytrace = false;
  bool canMeshshader = false;
  bool canVariableRate = false;
  GraphicsApi api = GraphicsApi::All;
  uint32_t apiVersion = 0;
  std::string apiVersionStr = "";
};
}
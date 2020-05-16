#pragma once

#include <higanbana/graphics/common/cpuimage.hpp>
#include <higanbana/core/filesystem/filesystem.hpp>
#include <higanbana/core/datastructures/proxy.hpp>
#include <string>
#include <string_view>
#include <optional>

namespace app
{
  higanbana::vector<std::string_view> splitToLines(std::string_view view);
  int extractInt(std::string_view line);
  void getRowData(std::string_view line, higanbana::vector<float>& out, int offset);
  std::optional<higanbana::CpuImage> readInfoFromOpenMapDataASC(higanbana::FileSystem& fs);
}
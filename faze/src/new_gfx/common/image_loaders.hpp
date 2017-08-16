#pragma once
#include <string>

namespace faze
{
  class FileSystem;
  class CpuImage;
  namespace textureUtils
  {
    CpuImage loadImageFromFilesystem(FileSystem& fs, std::string path);
  }
}
#pragma once
#include <string>

namespace higanbana
{
  class FileSystem;
  class CpuImage;
  namespace textureUtils
  {
    CpuImage loadImageFromFilesystem(FileSystem& fs, std::string path, bool flipVertically = true);
    void saveImageFromFilesystemPNG(FileSystem& fs, std::string path, CpuImage& toSave);
  }
}
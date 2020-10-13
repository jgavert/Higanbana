#include "higanbana/graphics/common/image_loaders.hpp"

#include "higanbana/graphics/common/cpuimage.hpp"
#include <higanbana/core/filesystem/filesystem.hpp>

#include <higanbana/core/profiling/profiling.hpp>

#define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace higanbana
{
  namespace textureUtils
  {
    CpuImage loadImageFromFilesystem(FileSystem& fs, std::string path, bool flipVertically)
    {
      std::string loadingFile = "loadImageFromFS " + path;
      HIGAN_CPU_BRACKET(loadingFile.c_str());
      //HIGAN_LOGi("Lol %s\n", path.c_str());
      HIGAN_ASSERT(fs.fileExists(path), ":D");

      auto view = fs.viewToFile(path);
      int imgX, imgY, channels;
      stbi_set_flip_vertically_on_load(flipVertically);
      stbi_uc* asd = nullptr;
      {
        HIGAN_CPU_BRACKET("stbi_load_from_memory");
        asd = stbi_load_from_memory(view.data(), static_cast<int>(view.size()), &imgX, &imgY, &channels, 4);
      }
      //HIGAN_SLOG("CpuImage", "loaded %dx%d %d\n", imgX, imgY, channels);

      CpuImage image(ResourceDescriptor()
        .setSize(int2(imgX, imgY))
        .setFormat(FormatType::Unorm8RGBA)
        .setName(path)
        .setUsage(ResourceUsage::GpuReadOnly));

      auto sr = image.subresource(0, 0);

      {
        HIGAN_CPU_BRACKET("memcpy");
        memcpy(sr.data(), asd, sr.size());
      }

      stbi_image_free(asd);

      return image;
    }
  }
}
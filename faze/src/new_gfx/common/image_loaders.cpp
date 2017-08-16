#include "image_loaders.hpp"

#include "core/src/filesystem/filesystem.hpp"
#include "faze/src/new_gfx/common/cpuimage.hpp"

#define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace faze
{
  namespace textureUtils
  {
    CpuImage loadImageFromFilesystem(FileSystem& fs, std::string path)
    {
      F_ASSERT(fs.fileExists(path), ":D");

      auto view = fs.viewToFile(path);
      int imgX, imgY, channels;
      stbi_set_flip_vertically_on_load(true);
      auto asd = stbi_load_from_memory(view.data(), static_cast<int>(view.size()), &imgX, &imgY, &channels, 4);
      F_LOG("%dx%d %d\n", imgX, imgY, channels);

      CpuImage image(ResourceDescriptor()
        .setSize({ imgX, imgY, 1 })
        .setDimension(FormatDimension::Texture2D)
        .setFormat(FormatType::Unorm8x4)
        .setName(path)
        .setUsage(ResourceUsage::GpuReadOnly));

      auto sr = image.subresource(0, 0);

      memcpy(sr.data(), asd, sr.size());

      stbi_image_free(asd);

      return image;
    }
  }
}
#pragma once

namespace faze
{
  namespace backend
  {
    struct DescriptorLayout
    {
        int byteAddressBuffers = 0;
        int texelBuffers = 0;
        int storageBuffers = 0;
        int textures = 0;
        int rwByteAddressBuffers = 0;
        int rwTexelBuffers = 0;
        int rwStorageBuffers = 0;
        int rwTextures = 0;
    };
  }
}

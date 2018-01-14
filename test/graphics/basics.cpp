#include "graphicsFixture.hpp"

TEST_F(Graphics, BufferCreation)
{
  auto bufferdesc = ResourceDescriptor()
    .setName("testBufferTarget")
    .setFormat(FormatType::Float32)
    .setWidth(32)
    .setDimension(FormatDimension::Buffer)
    .setUsage(ResourceUsage::GpuRW);

  auto buffer = gpu.createBuffer(bufferdesc);
  auto bufferSRV = gpu.createBufferSRV(buffer);
  auto bufferUAV = gpu.createBufferUAV(buffer);
}

TEST_F(Graphics, TextureCreation)
{
  auto testDesc = ResourceDescriptor()
    .setName("testTexture")
    .setFormat(FormatType::Unorm16RGBA)
    .setWidth(400)
    .setHeight(400)
    .setMiplevels(4)
    .setDimension(FormatDimension::Texture2D)
    .setUsage(ResourceUsage::RenderTargetRW);
  auto texture = gpu.createTexture(testDesc);
  auto texRtv = gpu.createTextureRTV(texture, ShaderViewDescriptor().setMostDetailedMip(0));
  auto texSrv = gpu.createTextureSRV(texture, ShaderViewDescriptor().setMostDetailedMip(0).setMipLevels(1));
  auto texUav = gpu.createTextureUAV(texture, ShaderViewDescriptor().setMostDetailedMip(0).setMipLevels(1));
}
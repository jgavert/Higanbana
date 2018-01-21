#include "gtest/gtest.h"
#include "faze/src/new_gfx/GraphicsCore.hpp"
using namespace faze;

TEST(GraphicsSubsystem, DeviceCreation)
{
  faze::FileSystem fs;
  faze::GraphicsSubsystem graphics(GraphicsApi::DX12, "faze");
  faze::GpuDevice gpu(graphics.createDevice(fs, graphics.getVendorDevice(VendorID::Nvidia)));
  EXPECT_TRUE(gpu.alive());
}

TEST(GraphicsSubsystem, Create2Devices)
{
  faze::FileSystem fs;
  faze::GraphicsSubsystem graphics(GraphicsApi::DX12, "faze");
  auto gpus = graphics.availableGpus();
  if (gpus.size() >= 2)
  {
    faze::GpuDevice gpu(graphics.createDevice(fs, gpus[0]));
    EXPECT_TRUE(gpu.alive());
    faze::GpuDevice gpu2(graphics.createDevice(fs, gpus[1]));
    EXPECT_TRUE(gpu2.alive());
  }
}
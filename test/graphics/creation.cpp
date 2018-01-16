#include "gtest/gtest.h"
#include "faze/src/new_gfx/GraphicsCore.hpp"
using namespace faze;

TEST(GraphicsSubsystem, DeviceCreation)
{
  faze::FileSystem fs;
  faze::GraphicsSubsystem graphics(GraphicsApi::DX12, "faze");
  faze::GpuDevice gpu(graphics.createDevice(fs, graphics.getVendorDevice(VendorID::Nvidia)));
  EXPECT_EQ(gpu.alive(), true);
}
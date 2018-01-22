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

TEST(GraphicsSubsystem, CrossAdapterShenigans_bufferCopies)
{
    faze::FileSystem fs;
    faze::GraphicsSubsystem graphics(GraphicsApi::DX12, "faze");
    auto gpus = graphics.availableGpus();
    if (gpus.size() >= 2)
    {
        faze::GpuDevice primaryGpu(graphics.createDevice(fs, gpus[0]));
        EXPECT_TRUE(primaryGpu.alive());
        faze::GpuDevice secondaryGpu(graphics.createDevice(fs, gpus[1]));
        EXPECT_TRUE(secondaryGpu.alive());

        {
            auto bufferDesc = ResourceDescriptor()
                .setName("test")
                .setFormat(FormatType::Uint32)
                .setWidth(64);

            auto bufferBySecondary = secondaryGpu.createBuffer(bufferDesc.setUsage(ResourceUsage::GpuReadOnly));
            auto sharedBuffer = primaryGpu.createSharedBuffer(secondaryGpu, bufferDesc.setName("shared").allowCrossAdapter().setUsage(ResourceUsage::GpuReadOnly));

            vector<unsigned> vertices;
            for (unsigned i = 0; i < 16; ++i)
            {
                vertices.push_back(static_cast<unsigned>(-1));
            }
            for (unsigned i = 0; i < 32; ++i)
            {
                vertices.push_back(i);
            }
            for (unsigned i = 0; i < 16; ++i)
            {
                vertices.push_back(static_cast<unsigned>(-1));
            }

            {
                bool WasReadback = false;

                CommandGraph secondaryTasks = secondaryGpu.createGraph();
                {
                    auto& node = secondaryTasks.createPass2("bufferGenerated", CommandGraphNode::NodeType::DMA);

                    auto verts = secondaryGpu.dynamicBuffer(makeMemView(vertices), FormatType::Uint32);

                    node.copy(bufferBySecondary, verts);
                    node.copy(sharedBuffer.getSecondaryBuffer(), bufferBySecondary);

                    node.readback(bufferBySecondary, 0, 64, [&WasReadback, vertices](MemView<uint8_t> view)
                    {
                        auto asd = reinterpret_memView<unsigned>(view);

                        for (unsigned i = 0; i < 64; ++i)
                        {
                            EXPECT_EQ(vertices[i], asd[i]);
                        }
                        EXPECT_EQ(64, asd.size());

                        WasReadback = false;
                    });
                }
                CommandGraph primaryTasks = primaryGpu.createGraph();
                {
                    auto& node = primaryTasks.createPass2("bufferDownloaded", CommandGraphNode::NodeType::DMA);
                    node.readback(sharedBuffer.getPrimaryBuffer(), 0, 64, [&WasReadback, vertices](MemView<uint8_t> view)
                    {
                        auto asd = reinterpret_memView<unsigned>(view);

                        for (unsigned i = 0; i < 64; ++i)
                        {
                            EXPECT_EQ(vertices[i], asd[i]);
                        }
                        EXPECT_EQ(64, asd.size());

                        WasReadback = true;
                    });
                }

                // so question is, how to make these tasks nice...

                secondaryGpu.submit(secondaryTasks);
                secondaryGpu.waitGpuIdle();

                primaryGpu.submit(primaryTasks);
                primaryGpu.waitGpuIdle();

                EXPECT_TRUE(WasReadback);
            }
        }
    }
}
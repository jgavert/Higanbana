#include <higanbana/graphics/GraphicsCore.hpp>
#include "graphics_config.hpp"
#include <string>
#include <catch2/catch_all.hpp>

using namespace higanbana;
using namespace higanbana::backend;

TEST_CASE("instance creation") {
  REQUIRE_NOTHROW(GraphicsSubsystem("higanbana", false));
}

TEST_CASE("device creation") {
  GraphicsSubsystem graphics("higanbana", false);
  FileSystem fs(TESTS_FILESYSTEM_PATH, FileSystem::MappingMode::NoMapping);
  {
    auto gpus = graphics.availableGpus();
    if (gpus.empty())
      return;
    auto desc = gpus[0];
    if (desc.api == GraphicsApi::All) {
      desc.api = GraphicsApi::DX12;
      REQUIRE_NOTHROW(graphics.createDevice(fs, desc));
    }
    desc.api = GraphicsApi::Vulkan;
    REQUIRE_NOTHROW(graphics.createDevice(fs, desc));
  }
}

#if 0
TEST_CASE("create 2 devices of same info") {
  GraphicsSubsystem graphics("higanbana", false);
  FileSystem fs(TESTS_FILESYSTEM_PATH, FileSystem::MappingMode::NoMapping);
  {
    auto gpus = graphics.availableGpus();
    if (gpus.empty())
      return;
    decltype(gpus) infos;
    auto desc = gpus[0];
    if (gpus[0].api == GraphicsApi::All || gpus[0].api == GraphicsApi::DX12) {
      // disabled for now
      desc.api = GraphicsApi::DX12;
      infos.push_back(desc);
      infos.push_back(desc);
      REQUIRE_NOTHROW(graphics.createGroup(fs, infos));
    }
  }
}
#endif

TEST_CASE("create buffer") {
  GraphicsSubsystem graphics("higanbana", false);
  FileSystem fs(TESTS_FILESYSTEM_PATH, FileSystem::MappingMode::NoMapping);
  {
    auto gpus = graphics.availableGpus();
    if (gpus.empty())
      return;
    auto desc = gpus[0];
    desc.api = GraphicsApi::Vulkan;
    auto dev = graphics.createDevice(fs, desc);

    {
      REQUIRE_NOTHROW(dev.createBuffer(ResourceDescriptor().setFormat(FormatType::Uint32).setCount(100)));
    }
  }
}
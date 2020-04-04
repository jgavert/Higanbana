#include <higanbana/graphics/GraphicsCore.hpp>
#include "graphics_config.hpp"
#include <string>
#include <catch2/catch.hpp>

using namespace higanbana;
using namespace higanbana::backend;

TEST_CASE("instance creation") {
  REQUIRE_NOTHROW(GraphicsSubsystem("higanbana", false));
}

TEST_CASE("device creation") {
  GraphicsSubsystem graphics("higanbana", false);
  FileSystem fs(TESTS_FILESYSTEM_PATH);
  {
    auto gpus = graphics.availableGpus();
    if (gpus.empty())
      return;
    decltype(gpus) infos;
    infos.push_back(gpus[0]);
    REQUIRE_NOTHROW(graphics.createDevice(fs, infos[0]));
  }
}

TEST_CASE("create 2 devices of same info") {
  GraphicsSubsystem graphics("higanbana", false);
  FileSystem fs(TESTS_FILESYSTEM_PATH);
  {
    auto gpus = graphics.availableGpus();
    if (gpus.empty())
      return;
    decltype(gpus) infos;
    infos.push_back(gpus[0]);
    infos.push_back(gpus[0]);
    REQUIRE_NOTHROW(graphics.createGroup(fs, infos));
  }
}

TEST_CASE("create buffer") {
  GraphicsSubsystem graphics("higanbana", false);
  FileSystem fs(TESTS_FILESYSTEM_PATH);
  {
    auto gpus = graphics.availableGpus();
    if (gpus.empty())
      return;
    decltype(gpus) infos;
    infos.push_back(gpus[0]);
    auto dev = graphics.createDevice(fs, infos[0]);

    {
      REQUIRE_NOTHROW(dev.createBuffer(ResourceDescriptor().setFormat(FormatType::Uint32).setCount(100)));
    }
  }
}
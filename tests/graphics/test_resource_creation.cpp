#include <higanbana/graphics/GraphicsCore.hpp>
#include "graphics_config.hpp"
#include <string>
#include <memory>

#include <catch2/catch.hpp>

using namespace higanbana;
using namespace higanbana::backend;

struct Resources
{
  higanbana::GraphicsSubsystem subsystem;
  higanbana::FileSystem fileSystem;
  higanbana::GpuGroup gpu;
  Resources()
    : subsystem("higanbana", false)
    , fileSystem(TESTS_FILESYSTEM_PATH)
  {
    auto gpus = subsystem.getVendorDevice(GraphicsApi::DX12); // TODO: make it so that it always tests both api at once
    gpu = subsystem.createGroup(fileSystem, {gpus});
  }
};

struct TestFixture {
  static bool _initialised;
  static std::shared_ptr<Resources> _g;
  TestFixture() {
    if (!_initialised) {
      _initialised = true;

      _g = std::make_shared<Resources>();
    }
    else
    {
      _g->gpu.waitGpuIdle();
    }
  }

  GpuGroup& gpu() {return _g->gpu; }
};

bool TestFixture::_initialised = false;

TEST_CASE_METHOD(TestFixture, "buffer creation") {
  REQUIRE_NOTHROW(gpu().createBuffer(ResourceDescriptor().setFormat(FormatType::Uint32).setCount(100)));
}
TEST_CASE_METHOD(TestFixture, "buffer creation1") {
  REQUIRE_NOTHROW(gpu().createBuffer(ResourceDescriptor().setFormat(FormatType::Uint32).setCount(100)));
}
TEST_CASE_METHOD(TestFixture, "buffer creation2") {
  REQUIRE_NOTHROW(gpu().createBuffer(ResourceDescriptor().setFormat(FormatType::Uint32).setCount(100)));
}
TEST_CASE_METHOD(TestFixture, "buffer creation3") {
  REQUIRE_NOTHROW(gpu().createBuffer(ResourceDescriptor().setFormat(FormatType::Uint32).setCount(100)));
}
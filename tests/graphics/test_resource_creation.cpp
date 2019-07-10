#include "tests/graphics/test_resource_creation.hpp"


SResources::SResources()
  : subsystem("higanbana", false)
  , fileSystem(TESTS_FILESYSTEM_PATH)
{
  auto gpus = subsystem.getVendorDevice(GraphicsApi::DX12); // TODO: make it so that it always tests both api at once
  gpu = subsystem.createGroup(fileSystem, {gpus});
}

static std::shared_ptr<SResources> _g;

TestFixture::TestFixture() {
  if (!_initialised) {
    _initialised = true;

    _g = std::make_shared<SResources>();
  }
  else
  {
    _g->gpu.waitGpuIdle();
  }
}

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
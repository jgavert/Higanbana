#include "tests/graphics/graphics_config.hpp"
#include <catch2/catch_all.hpp>

TEST_CASE_METHOD(GraphicsFixture, "buffer creation") {
  REQUIRE_NOTHROW(gpu().createBuffer(ResourceDescriptor().setFormat(FormatType::Uint32).setCount(100)));
}
TEST_CASE_METHOD(GraphicsFixture, "buffer creation1") {
  REQUIRE_NOTHROW(gpu().createBuffer(ResourceDescriptor().setFormat(FormatType::Uint32).setCount(100)));
}
TEST_CASE_METHOD(GraphicsFixture, "buffer creation2") {
  REQUIRE_NOTHROW(gpu().createBuffer(ResourceDescriptor().setFormat(FormatType::Uint32).setCount(100)));
}
TEST_CASE_METHOD(GraphicsFixture, "buffer creation3") {
  REQUIRE_NOTHROW(gpu().createBuffer(ResourceDescriptor().setFormat(FormatType::Uint32).setCount(100)));
}
#pragma once
#include <higanbana/graphics/GraphicsCore.hpp>
#define TESTS_FILESYSTEM_PATH "/../../tests/data"

using namespace higanbana;
using namespace higanbana::backend;

struct SResources
{
  higanbana::GraphicsSubsystem subsystem;
  higanbana::FileSystem fileSystem;
  higanbana::GpuGroup gpu;
  SResources();
};

extern std::shared_ptr<SResources> _g;

struct GraphicsFixture {
  static bool _initialised;
  GraphicsFixture();
  GpuGroup& gpu();
};
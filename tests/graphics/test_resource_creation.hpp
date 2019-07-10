#include <higanbana/graphics/GraphicsCore.hpp>
#include "graphics_config.hpp"
#include <string>
#include <memory>

#include <catch2/catch.hpp>

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

struct TestFixture {
  static bool _initialised;
  TestFixture();

  GpuGroup& gpu() {return _g->gpu; }
};
#pragma once

#include "gtest/gtest.h"

#include "core/src/system/LBS.hpp"
#include "core/src/system/time.hpp"
#include "core/src/system/logger.hpp"
#include "faze/src/new_gfx/GraphicsCore.hpp"

using namespace faze;

namespace {
  // To use a test fixture, derive a class from testing::Test.
  class Graphics : public testing::Test {
  protected:

    faze::FileSystem fs;
    faze::GraphicsSubsystem graphics;
    faze::GpuDevice gpu;

    Graphics()
      :graphics(GraphicsApi::DX12, "faze")
      , gpu(graphics.createDevice(fs, graphics.getVendorDevice(VendorID::Amd)))
    {
    }
    // You should make the members protected s.t. they can be
    // accessed from sub-classes.

    // virtual void SetUp() will be called before each test is run.  You
    // should define it if you need to initialize the variables.
    // Otherwise, this can be skipped.
    virtual void SetUp() {
    }

    // virtual void TearDown() will be called after each test is run.
    // You should define it if there is cleanup work to do.  Otherwise,
    // you don't have to provide it.
    //
    // virtual void TearDown() {
    // }
  };
}
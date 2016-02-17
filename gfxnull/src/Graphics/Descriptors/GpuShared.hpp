#pragma once

struct GpuShared
{
  bool shared;
  GpuShared() :shared(false) {}
  GpuShared(bool shared) :shared(shared) {}
};

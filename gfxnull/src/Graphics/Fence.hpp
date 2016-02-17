#pragma once

class GpuFence
{
private:
  void* m_fence;
  GpuFence(void* mFence);
public:
  GpuFence() {}
  bool isSignaled();
  void wait();
};

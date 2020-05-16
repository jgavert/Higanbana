#pragma once

#include "higanbana/core/system/ringbuffer.hpp"
#include "higanbana/core/math/math.hpp"
#include "higanbana/core/system/HighResClock.hpp"

#include <chrono>
#include <thread>
#include <iostream>
#include <algorithm>

namespace higanbana
{
  class Timer 
  {
  public:
    Timer();
    int64_t reset();
    int64_t timeFromLastReset();
    int64_t timeMicro();
  private:
    timepoint start;
  };

  class WTime
  {
  public:
    WTime();
    void firstTick();
    void startFrame();
    void tick();
    float getCurrentFps();
    float getMaxFps();
    float getAverageFps();
    float getFrameTimeDelta();
    float getAverageFrametime();
    float getframetime_high();
    float getframetime_low();
    int64_t getCurrentNano();
    float getFTime();
    void printStatistics();
    int64_t getFrame();
    float3 analyzeFrames();
  private:
    float interpolateFpsFromNanoseconds(int64_t time);
    float deltatime;
    float timeFromStart;
    int64_t frames;
    int64_t frametime_high;
    int64_t frametime_low;
    int64_t frametime_current;
    timepoint start;
    timepoint timepoint_last;
    RingBuffer<int64_t, 30> lastFrameTimes;
  };
}

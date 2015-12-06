#pragma once

#include "ringbuffer.hpp"
#include "../math/vec_templated.hpp"

#include <chrono>
#include <thread>
#include <iostream>
#include <algorithm>

#ifdef WIN64
#include "HighResClock.hpp"
typedef HighResClock HighPrecisionClock;
typedef std::chrono::time_point<HighResClock> timepoint;
#else
typedef std::chrono::high_resolution_clock HighPrecisionClock;
typedef std::chrono::high_resolution_clock::time_point timepoint;
#endif


namespace faze
{

  class WTime
  {
  public:
    WTime();
    ~WTime();
    void firstTick();
    void startFrame();
    void tick();
    float getCurrentFps();
    float getAverageFps();
    float getFrameTimeDelta();
    float getAverageFrametime();
    float getframetime_high();
    float getframetime_low();
    int64_t getCurrentNano();
    void printStatistics();
    int64_t getFrame();
    vec3 analyzeFrames();
  private:
    float interpolateFpsFromNanoseconds(int64_t time);
    float deltatime;
    float fps;
    int64_t frames;
    float fpsHigh;
    float fpsLow;
    int64_t frametime_high;
    int64_t frametime_low;
    int64_t frametime_median;
    int64_t frametime_current;
    timepoint start;
    timepoint timepoint_last;
    RingBuffer<int64_t, 30> lastFrameTimes;
  };
}

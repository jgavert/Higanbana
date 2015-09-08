#include "time.hpp"
#include "../global_debug.hpp"
using namespace faze;

WTime::WTime() : start(HighPrecisionClock::now())
{
  frametime_high = 0;
  frametime_low = INT64_MAX;
  lastFrameTimes.forEach([](int64_t& data)
  {
    data = 0;
  });
}

WTime::~WTime()
{
}

void WTime::firstTick() {
  start = HighPrecisionClock::now();
  timepoint_last = HighPrecisionClock::now();
  deltatime = 0.0f;
  frametime_high = 0;
  frametime_low = INT64_MAX;
  //frametime_current = 0;
  frames = 0;
}

void WTime::startFrame()
{
  start = HighPrecisionClock::now();
  timepoint_last = HighPrecisionClock::now();
}


void WTime::tick()
{
  frames++;
  auto timepoint_now = HighPrecisionClock::now();
  deltatime = (float)std::chrono::duration_cast<std::chrono::microseconds>(timepoint_now - timepoint_last).count();
  frametime_current = std::chrono::duration_cast<std::chrono::nanoseconds>(timepoint_now - timepoint_last).count();
  //deltatime *= 0.01f;  //seconds
  timepoint_last = timepoint_now;
  lastFrameTimes.push_back(frametime_current);
  if (frametime_current < frametime_low)
  {
    frametime_low = frametime_current;
  }
  else if (frametime_current > frametime_high && frames > 1)
  {
    frametime_high = frametime_current;
    //std::cout << frametime_current* 0.000001f << "ms\n";
  }
}

float WTime::interpolateFpsFromNanoseconds(int64_t time) {
  return  1000000000.f / time;
}

float WTime::getCurrentFps() {
  int64_t count = 0;
  lastFrameTimes.forEach([&](int64_t& data)
  {
    count += data;
  });
  return static_cast<float>(count) / static_cast<float>(lastFrameTimes.size())* 0.000001f;
}

vec3 WTime::analyzeFrames() {
  int64_t count = 0;
  int64_t high = 0;
  int64_t low = INT64_MAX;
  lastFrameTimes.forEach([&](int64_t& data)
  {
    count += data;
    high = std::max(data, high);
    low = std::min(data, low);
  });
  return{ static_cast<float>(count) / static_cast<float>(lastFrameTimes.size())* 0.000001f, low*0.000001f, high*0.000001f };
}

float WTime::getAverageFps()
{
  return (float)frames / (float)std::chrono::duration_cast<std::chrono::seconds>(timepoint_last - start).count();
}
float WTime::getFrameTimeDelta()
{
  return deltatime*0.000001f;
}

float WTime::getAverageFrametime() {
  return (float)std::chrono::duration_cast<std::chrono::milliseconds>(timepoint_last - start).count() / (float)frames;
}

float WTime::getframetime_high() {
  return frametime_high * 0.000001f;
}

float WTime::getframetime_low() {
  return frametime_low * 0.000001f;
}

void WTime::printStatistics() {
  F_LOG("Time running: %zu seconds, ", std::chrono::duration_cast<std::chrono::seconds>(timepoint_last - start).count());
  F_LOG("frames: %zu\n", frames);
  F_LOG("Fps(min, aveg, high): %f", interpolateFpsFromNanoseconds(frametime_high));
  F_LOG(" / %f / %f\n", getAverageFps(), interpolateFpsFromNanoseconds(frametime_low));
  F_LOG("frametimes(min, aveg, high): %fms / %f", getframetime_low(), getAverageFrametime());
  F_LOG("ms / %fms \n", getframetime_high());
  /*
  std::cout << "Time running: " << std::chrono::duration_cast<std::chrono::seconds>(timepoint_last - start).count() << " seconds, ";
  std::cout << "frames: " << frames << std::endl;
  std::cout << "Fps(min, aveg, high): " << interpolateFpsFromNanoseconds(frametime_high);
  std::cout << " / " << getAverageFps() << " / " << interpolateFpsFromNanoseconds(frametime_low) << std::endl;
  std::cout << "frametimes(min, aveg, high): " << getframetime_low() << "ms / " << getAverageFrametime();
  std::cout << "ms / " << getframetime_high() << "ms " << std::endl;
  */
}

int64_t WTime::getCurrentMicro()
{
  return frametime_current;
}

int64_t WTime::getFrame()
{
  return frames;
}

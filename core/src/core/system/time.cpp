#include "core/system/time.hpp"
#include "core/global_debug.hpp"
using namespace faze;

WTime::WTime() : start(HighPrecisionClock::now())
{
  frametime_high = 0;
  frametime_low = INT64_MAX;
  lastFrameTimes.forEach([](int64_t& data)
  {
    data = 0;
  });
  startFrame();
}

void WTime::firstTick() {
  start = HighPrecisionClock::now();
  timepoint_last = HighPrecisionClock::now();
  deltatime = 0.0f;
  frametime_high = 0;
  frametime_low = INT64_MAX;
  frames = 0;
}

void WTime::startFrame()
{
  start = HighPrecisionClock::now();
  timepoint_last = HighPrecisionClock::now();
  timeFromStart = 0.f;
}

void WTime::tick()
{
  frames++;
  auto timepoint_now = HighPrecisionClock::now();
  deltatime = (float)std::chrono::duration_cast<std::chrono::microseconds>(timepoint_now - timepoint_last).count();
  frametime_current = std::chrono::duration_cast<std::chrono::nanoseconds>(timepoint_now - timepoint_last).count();
  timepoint_last = timepoint_now;
  lastFrameTimes.push_back(frametime_current);
  if (frametime_current < frametime_low)
  {
    frametime_low = frametime_current;
  }
  else if (frametime_current > frametime_high && frames > 1)
  {
    frametime_high = frametime_current;
  }
  timeFromStart += deltatime*0.001;
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

float WTime::getMaxFps() {
    int64_t count = 999999999999999999ll;
    lastFrameTimes.forEach([&](int64_t& data)
    {
        count = std::min(count, data);
    });
    return static_cast<float>(count)* 0.000001f;
}

float3 WTime::analyzeFrames() {
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

void WTime::printStatistics()
{
  F_SLOG("WTime", "Time running: %.3f seconds, ", std::chrono::duration_cast<std::chrono::milliseconds>(timepoint_last - start).count() / 1000.f);
  F_LOG_UNFORMATTED("frames: %zu\n", frames);
  F_SLOG("WTime", "Fps(min, aveg, high): %f", interpolateFpsFromNanoseconds(frametime_high));
  F_LOG_UNFORMATTED(" / %f / %f\n", getAverageFps(), interpolateFpsFromNanoseconds(frametime_low));
  F_SLOG("WTime", "frametimes(min, aveg, high): %fms / %f", getframetime_low(), getAverageFrametime());
  F_LOG_UNFORMATTED("ms / %fms \n", getframetime_high());
}

int64_t WTime::getCurrentNano()
{
  return frametime_current;
}

float WTime::getFTime()
{
  return timeFromStart * 0.001;
}

int64_t WTime::getFrame()
{
  return frames;
}
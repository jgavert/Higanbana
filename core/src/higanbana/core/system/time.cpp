#include "higanbana/core/system/time.hpp"
#include "higanbana/core/global_debug.hpp"

namespace higanbana
{
TimeStatistics::TimeStatistics() {
  for (int i = 0; i < valuesStored.max_size(); ++i) {
    valuesStored[i] = 0;
  }
}
void TimeStatistics::push(int64_t nanoseconds){
  valuesStored.push_back(nanoseconds);
}
float3 TimeStatistics::minAvegMaxMS(){
  int64_t count = 0;
  int64_t high = 0;
  int64_t low = INT64_MAX;
  valuesStored.forEach([&](int64_t& data)
  {
    count += data;
    high = std::max(data, high);
    low = std::min(data, low);
  });
  if (count == 0)
    return float3(0,0,0);
  return{ static_cast<float>(count) / static_cast<float>(valuesStored.size())* 0.000001f, low*0.000001f, high*0.000001f };
}

Timer::Timer() 
  : start(HighPrecisionClock::now())
{

}

int64_t Timer::reset()
{
  auto current = HighPrecisionClock::now();
  auto val = std::chrono::duration_cast<std::chrono::nanoseconds>(current - start).count();
  start = current;
  return val;
}

int64_t Timer::timeFromLastReset()
{
  auto current = HighPrecisionClock::now();
  return std::chrono::duration_cast<std::chrono::nanoseconds>(current - start).count();
}
int64_t Timer::timeMicro()
{
  auto current = HighPrecisionClock::now();
  return std::chrono::duration_cast<std::chrono::microseconds>(current - start).count();
}



WTime::WTime() : start(HighPrecisionClock::now())
{
  frames = 0;
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
  int64_t dataSize = 0;
  lastFrameTimes.forEach([&](int64_t& data)
  {
    count += data;
    dataSize++;
  });
  return static_cast<float>(count) / static_cast<float>(dataSize)* 0.000001f;
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
  // this can return too large values sometimes.
  return std::min(0.5f, deltatime*0.000001f);
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
  HIGAN_SLOG("WTime", "Time running: %.3f seconds, ", std::chrono::duration_cast<std::chrono::milliseconds>(timepoint_last - start).count() / 1000.f);
  HIGAN_LOG_UNFORMATTED("frames: %zu\n", frames);
  HIGAN_SLOG("WTime", "Fps(min, aveg, high): %f", interpolateFpsFromNanoseconds(frametime_high));
  HIGAN_LOG_UNFORMATTED(" / %f / %f\n", getAverageFps(), interpolateFpsFromNanoseconds(frametime_low));
  HIGAN_SLOG("WTime", "frametimes(min, aveg, high): %fms / %f", getframetime_low(), getAverageFrametime());
  HIGAN_LOG_UNFORMATTED("ms / %fms \n", getframetime_high());
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
}
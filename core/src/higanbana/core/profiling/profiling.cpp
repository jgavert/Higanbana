#include "higanbana/core/profiling/profiling.hpp"
#include "higanbana/core/platform/definitions.hpp"

#if defined(HIGANBANA_PLATFORM_WINDOWS)
#include <windows.h>
#if ! defined(USE_PIX)
#define USE_PIX
#endif
#if ! defined(USE_PIX_SUPPORTED_ARCHITECTURE)
#define USE_PIX_SUPPORTED_ARCHITECTURE
#endif
#include <WinPixEventRuntime/pix3.h>
#endif
#include <nlohmann/json.hpp>

namespace higanbana
{
namespace profiling
{
std::array<ThreadProfileData, 1024> s_allThreadsProfilingData;
std::atomic<int> s_myIndex;
thread_local ThreadProfileData* my_profile_data = nullptr;
thread_local int my_thread_id = 0;
thread_local int64_t previousBegin = std::numeric_limits<int64_t>::min();
thread_local int64_t nextDuration = 0;

int64_t getCurrentTime() {
  int64_t value = std::chrono::time_point_cast<std::chrono::nanoseconds>(HighPrecisionClock::now()).time_since_epoch().count();
  if (value <= previousBegin) {
    value = previousBegin + 1000;
    nextDuration -= 1000;
  }
  else {
    nextDuration = 0;
  }
  return value;
}

ProfilingScope::ProfilingScope(const char* str)
  : name(str)
  , start(getCurrentTime())
{
#if defined(HIGANBANA_PLATFORM_WINDOWS)
  PIXBeginEvent(0ull, str);
#endif
  if (!my_profile_data)
  {
    int newIdx = s_myIndex.fetch_add(1);
    my_thread_id = newIdx;
    s_allThreadsProfilingData[newIdx] = ThreadProfileData();
    my_profile_data = &s_allThreadsProfilingData[newIdx];
  }
  // start
}
  ProfilingScope::~ProfilingScope(){
  // end
#if defined(HIGANBANA_PLATFORM_WINDOWS)
  PIXEndEvent();
#endif
  int64_t duration = getCurrentTime() - start;
  if (my_profile_data->canWriteProfile())
    my_profile_data->allBrackets.push_back(ProfileData{name, start, duration + nextDuration});
}

void writeGpuBracketData(int gpuid, int queue, std::string_view view, int64_t time, int64_t dur)
{
  if (!my_profile_data)
  {
    int newIdx = s_myIndex.fetch_add(1);
    my_thread_id = newIdx;
    s_allThreadsProfilingData[newIdx] = ThreadProfileData();
    my_profile_data = &s_allThreadsProfilingData[newIdx];
  }
  if (my_profile_data->canWriteProfile())
    my_profile_data->gpuBrackets.push_back(GPUProfileData{gpuid, queue, std::string(view), time, dur});
}

nlohmann::json writeEvent(std::string_view view, int64_t time, int64_t dur, int tid)
{
  nlohmann::json j;
  // "name": "myFunction", "cat": "foo", "ph": "B", "ts": 123, "pid": 2343, "tid": 2347
  j["name"] = view;
  j["cat"] = "foo";
  j["ph"] = "X";
  j["ts"] = double(time) / 1000.0;
  j["pid"] = "higanbana";
  j["dur"] = std::max(double(0.001), double(dur) / 1000.0);
  j["tid"] = "Thread " + std::to_string(tid);

  return j;
}
nlohmann::json writeEventGPU(std::string_view view, int64_t time, int64_t dur, int tid, int queue)
{
  nlohmann::json j;
  // "name": "myFunction", "cat": "foo", "ph": "B", "ts": 123, "pid": 2343, "tid": 2347
  j["name"] = view;
  j["cat"] = "foo";
  j["ph"] = "X";
  j["ts"] = double(time) / 1000.0;
  j["pid"] = "higanbana";
  j["dur"] = double(dur) / 1000.0;
  j["tid"] = "GPU " + std::to_string(tid) + " Queue " + std::to_string(queue);

  return j;
}

void writeProfilingData(higanbana::FileSystem& fs)
{
  nlohmann::json j;
  higanbana::vector<nlohmann::json> events;
  // last 5 seconds
  //auto lastEvent = s_allThreadsProfilingData[0].allBrackets[s_allThreadsProfilingData[0].allBrackets.end_ind()];
  if (s_allThreadsProfilingData[0].allBrackets.empty())
    return;
  auto lastEvent = s_allThreadsProfilingData[0].allBrackets.back();
  auto lastSeconds = 0; //lastEvent.begin + lastEvent.duration - 10000000000ll;
  for (int i = 0; i < s_myIndex; ++i)
  {
    higanbana::profiling::ProfileData previousEvent;
    int eventCount = 0;
    s_allThreadsProfilingData[i].canWriteProfilingData->store(false, std::memory_order::seq_cst);
    auto cpuBrackets = s_allThreadsProfilingData[i].allBrackets;
    for (const auto& event : cpuBrackets){
      //if (event.begin > lastSeconds)
        events.push_back(writeEvent(event.name, event.begin, event.duration, i));
    }//);
    auto gpuBrackets =  s_allThreadsProfilingData[i].gpuBrackets;
    for (const auto& event : gpuBrackets){
    //s_allThreadsProfilingData[i].gpuBrackets.forEach([&](const GPUProfileData& event)
      //if (event.begin > lastSeconds)
        events.push_back(writeEventGPU(event.name, event.begin, event.duration, event.gpuID, event.queue));
    }//);
    s_allThreadsProfilingData[i].canWriteProfilingData->store(true);
  }
  j["traceEvents"] = events;
  std::string output = j.dump();
  fs.writeFile("/profiling.trace", higanbana::makeByteView(output.data(), output.size()));
}
}
}
#include "higanbana/core/profiling/profiling.hpp"
#include "higanbana/core/filesystem/filesystem.hpp"
#include "higanbana/core/platform/definitions.hpp"

#include <memory>
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
GlobalProfilingThing::GlobalProfilingThing(int threadCount) :myIndex(0), enabled(false) {
  allThreadsProfilingData.resize(threadCount);
}
GlobalProfilingThing* s_profiling = nullptr;
thread_local ThreadProfileData* my_profile_data = nullptr;
thread_local int my_thread_id = 0;
thread_local int64_t previousBegin = std::numeric_limits<int64_t>::min();
thread_local int64_t seen = 0;
thread_local int64_t nextDuration = 0;

std::unique_ptr<GlobalProfilingThing> initializeProfiling(int threadCount) {
  auto profiling = std::make_unique<GlobalProfilingThing>(threadCount);
  s_profiling = profiling.get();
  return profiling;
}
void enableProfiling() {
  for (auto&& data : s_profiling->allThreadsProfilingData)
  {
    data.reset();
  }
  s_profiling->enabled = true;
}
void disableProfiling() {
  s_profiling->enabled = false;
  s_profiling = nullptr;
}

int64_t getCurrentTime() {
  int64_t value = std::chrono::time_point_cast<std::chrono::nanoseconds>(HighPrecisionClock::now()).time_since_epoch().count();
  if (value == previousBegin) {
    value = previousBegin + seen*100000;
    seen++;
    nextDuration = 0;
  }
  else {
    nextDuration = 0;
    seen = 0;
    previousBegin = value;
  }

  return value;
}

ProfilingScope::ProfilingScope(const char* str)
  : name(str)
  , start(getCurrentTime())
{
#if defined(HIGANBANA_PLATFORM_WINDOWS)
  PIXBeginEvent(0ull, "%s", str);
#endif
  if (!s_profiling || !s_profiling->enabled)
    return;
  if (!my_profile_data)
  {
    int newIdx = s_profiling->myIndex.fetch_add(1);
    my_thread_id = newIdx;
    s_profiling->allThreadsProfilingData[newIdx] = ThreadProfileData();
    my_profile_data = &s_profiling->allThreadsProfilingData[newIdx];
  }
  // start
}
  ProfilingScope::~ProfilingScope(){
#if defined(HIGANBANA_PLATFORM_WINDOWS)
  PIXEndEvent();
#endif
  if (!s_profiling || !s_profiling->enabled)
    return;
  // end
  int64_t duration = getCurrentTime() - start;
  if (my_profile_data->canWriteProfile())
    my_profile_data->allBrackets.push_back(ProfileData{name, start, duration + nextDuration});
}

void writeGpuBracketData(int gpuid, int queue, std::string_view view, int64_t time, int64_t dur)
{
  if (!s_profiling || !s_profiling->enabled)
    return;
  if (!my_profile_data)
  {
    int newIdx = s_profiling->myIndex.fetch_add(1);
    my_thread_id = newIdx;
    s_profiling->allThreadsProfilingData[newIdx] = ThreadProfileData();
    my_profile_data = &s_profiling->allThreadsProfilingData[newIdx];
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

void writeProfilingData(higanbana::FileSystem& fs, GlobalProfilingThing* profiling)
{
  nlohmann::json j;
  higanbana::vector<nlohmann::json> events;
  // last 5 seconds
  //auto lastEvent = s_allThreadsProfilingData[0].allBrackets[s_allThreadsProfilingData[0].allBrackets.end_ind()];
  if (!my_profile_data)
    return;
  if (profiling->allThreadsProfilingData[0].allBrackets.size() == 0)
    return;
  auto lastEvent = profiling->allThreadsProfilingData[0].allBrackets.back();
  auto lastSeconds = (lastEvent.begin + lastEvent.duration) - 3ull * 1000ull * 1000ull * 1000ull;
  for (int i = 0; i < profiling->myIndex; ++i)
  {
    higanbana::profiling::ProfileData previousEvent;
    int eventCount = 0;
    profiling->allThreadsProfilingData[i].canWriteProfilingData->store(false, std::memory_order::seq_cst);
    auto& cpuBrackets = profiling->allThreadsProfilingData[i].allBrackets;
    for (const auto& event : cpuBrackets){
    //cpuBrackets.forEach([&](const ProfileData& event) {
      //if (event.begin > lastSeconds)
        events.push_back(writeEvent(event.name, event.begin, event.duration, i));
    }//);
    auto& gpuBrackets =  profiling->allThreadsProfilingData[i].gpuBrackets;
    for (const auto& event : gpuBrackets){
    //gpuBrackets.forEach([&](const GPUProfileData& event) {
      //if (event.begin > lastSeconds)
        events.push_back(writeEventGPU(event.name, event.begin, event.duration, event.gpuID, event.queue));
    }//);
    profiling->allThreadsProfilingData[i].canWriteProfilingData->store(true);
  }
  j["traceEvents"] = events;
  std::string output = j.dump();
  fs.writeFile("/data/profiling.trace", higanbana::makeByteView(output.data(), output.size()));
}
}
}
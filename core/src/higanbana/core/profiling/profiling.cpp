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

namespace higanbana
{
namespace profiling
{
static std::array<ThreadProfileData, 1024> s_allThreadsProfilingData;
static std::atomic<int> s_myIndex;
thread_local ThreadProfileData* my_profile_data = nullptr;
thread_local int my_thread_id = 0;

ProfilingScope::ProfilingScope(const char* str)
  : name(str)
  , begin(HighPrecisionClock::now())
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
  int64_t now_ns = std::chrono::time_point_cast<std::chrono::nanoseconds>(begin).time_since_epoch().count();
  timepoint new_tp = HighPrecisionClock::now();
  int64_t duration = std::chrono::duration_cast<std::chrono::nanoseconds>(new_tp - begin).count();
  my_profile_data->allBrackets.push_back({name, now_ns, duration});
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
  j["dur"] = double(dur) / 1000.0;
  j["tid"] = tid;

  return j;
}

void writeProfilingData(higanbana::FileSystem& fs)
{
  nlohmann::json j;
  higanbana::vector<nlohmann::json> events;
  for (int i = 0; i < s_myIndex; ++i)
  {
    for (auto&& event : s_allThreadsProfilingData[i].allBrackets)
    {
      events.push_back(writeEvent(event.name, event.begin, event.duration, i));
    }
  }
  j["traceEvents"] = events;
  std::string output = j.dump();
  fs.writeFile("/profiling.trace", higanbana::makeByteView(output.data(), output.size()));
}
}
}
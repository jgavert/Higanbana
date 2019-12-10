#pragma once

#include "higanbana/core/datastructures/proxy.hpp"
#include "higanbana/core/system/HighResClock.hpp"
#include "higanbana/core/filesystem/filesystem.hpp"
#include <atomic>
#include <nlohmann/json.hpp>

namespace higanbana
{
namespace profiling
{

struct ProfileData
{
  std::string name;
  int64_t begin;
  int64_t duration;
};

class ThreadProfileData
{
  public:
  vector<ProfileData> allBrackets;
};
extern std::array<ThreadProfileData, 1024> s_allThreadsProfilingData;
extern std::atomic<int> s_myIndex;
extern thread_local ThreadProfileData* my_profile_data;
extern thread_local int my_thread_id;

class ProfilingScope
{
  const char* name;
  timepoint begin;

  public:
  ProfilingScope(const char* name);
  ~ProfilingScope();
};
nlohmann::json writeEvent(std::string_view view, int64_t time, int64_t dur, int tid);
void writeProfilingData(higanbana::FileSystem& fs);
}
}
#if 0
#define HIGAN_CONCAT(a, b) a ## b
#define HIGAN_CONCAT_HELPER(a, b) HIGAN_CONCAT(a, b)
#define HIGAN_CPU_BRACKET(name) auto HIGAN_CONCAT_HELPER(HIGAN_CONCAT_HELPER(profile_scope, __COUNTER__), __LINE__) = higanbana::profiling::ProfilingScope(name)
#else
#define HIGAN_CPU_BRACKET(name)
#endif
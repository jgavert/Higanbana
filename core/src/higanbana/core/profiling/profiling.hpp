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

class ProfilingBracket
{
  const char* name;
  timepoint begin;

  public:
  ProfilingBracket(const char* name);
  ~ProfilingBracket();
};
nlohmann::json writeEvent(std::string_view view, int time, int dur, int tid);
void writeProfilingData(higanbana::FileSystem& fs);
}
}
#define CONCAT(a, b) a ## b
#define HIGAN_CPU_BRACKET(name) higanbana::profiling::ProfilingBracket(name)
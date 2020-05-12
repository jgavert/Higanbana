#pragma once

#include "higanbana/core/datastructures/proxy.hpp"
#include "higanbana/core/system/HighResClock.hpp"
#include "higanbana/core/filesystem/filesystem.hpp"
#include "higanbana/core/system/ringbuffer.hpp"
#include <atomic>

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

struct GPUProfileData
{
  int gpuID;
  int queue;
  std::string name;
  int64_t begin;
  int64_t duration;
};

class ThreadProfileData
{
  public:
  ThreadProfileData():canWriteProfilingData(std::make_shared<std::atomic_bool>(true)) {}
  //RingBuffer<ProfileData, 10240> allBrackets;
  //RingBuffer<GPUProfileData, 10240> gpuBrackets;
  vector<ProfileData> allBrackets;
  vector<GPUProfileData> gpuBrackets;
  std::shared_ptr<std::atomic_bool> canWriteProfilingData;
  inline bool canWriteProfile() const { return canWriteProfilingData->load(std::memory_order::seq_cst);}
};
extern std::array<ThreadProfileData, 1024> s_allThreadsProfilingData;
extern std::atomic<int> s_myIndex;
extern thread_local ThreadProfileData* my_profile_data;
extern thread_local int my_thread_id;

class ProfilingScope
{
  const char* name;
  //timepoint begin;
  int64_t start;

  public:
  ProfilingScope(const char* name);
  ~ProfilingScope();
};
void writeGpuBracketData(int gpuid, int queue, std::string_view view, int64_t time, int64_t dur);
//nlohmann::json writeEvent(std::string_view view, int64_t time, int64_t dur, int tid);
void writeProfilingData(higanbana::FileSystem& fs);
}
}
#if 0
#define HIGAN_CONCAT(a, b) a ## b
#define HIGAN_CONCAT_HELPER(a, b) HIGAN_CONCAT(a, b)
#define HIGAN_CPU_BRACKET(name) auto HIGAN_CONCAT_HELPER(HIGAN_CONCAT_HELPER(profile_scope, __COUNTER__), __LINE__) = higanbana::profiling::ProfilingScope(name)
#define HIGAN_CPU_FUNCTION_SCOPE() auto HIGAN_CONCAT_HELPER(HIGAN_CONCAT_HELPER(profile_scope, __COUNTER__), __LINE__) = higanbana::profiling::ProfilingScope(__FUNCTION__)
#define HIGAN_GPU_BRACKET_FULL(gpuid, queue, view, time, dur) higanbana::profiling::writeGpuBracketData(gpuid, queue, view, time, dur)
#else
#define HIGAN_CPU_BRACKET(name)
#define HIGAN_CPU_FUNCTION_SCOPE()
#define HIGAN_GPU_BRACKET_FULL(gpuid, queue, view, time, dur) 
#endif
#pragma once

#include <higanbana/core/datastructures/proxy.hpp>
#include <cstdint>

namespace higanbana
{
  class Timestamp
  {
  public:
    uint64_t begin;
    uint64_t end;
    Timestamp();
    Timestamp(uint64_t begin, uint64_t end);
    void start();
    void stop();
    uint64_t nanoseconds() const;
  };

  struct GraphNodeTiming
  {
    std::string nodeName;
    Timestamp cpuTime;
    Timestamp gpuTime;
  };

  struct CommandListTiming
  {
    Timestamp cpuBackendTime;
    Timestamp barrierSolve;
    Timestamp fillNativeList;
    Timestamp gpuTime;
    vector<GraphNodeTiming> nodes;
  };

  struct SubmitTiming
  {
    uint64_t id;
    int listsCount;
    Timestamp timeBeforeSubmit;
    Timestamp submitCpuTime;
    Timestamp fillCommandLists;
    Timestamp graphSolve;
    Timestamp submitSolve;
    vector<CommandListTiming> lists;
  };
};
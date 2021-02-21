#pragma once

#include "higanbana/graphics/common/resources.hpp"
#include "higanbana/graphics/common/resources/graphics_api.hpp"
#include <higanbana/core/datastructures/vector.hpp>
#include <cstdint>
#include <string>

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
    float milliseconds() const;
  };

  struct GraphNodeTiming
  {
    std::string nodeName;
    Timestamp cpuTime;
    Timestamp gpuTime;
    int dispatches;
    int draws;
    size_t cpuSizeBytes;
  };

  struct CommandListTiming
  {
    QueueType type;
    Timestamp cpuBackendTime;
    Timestamp barrierAdd;
    Timestamp barrierSolveLocal;
    Timestamp barrierSolveGlobal;
    Timestamp fillNativeList;
    Timestamp constantsDmaTime;
    size_t constantsTransferredBytes;
    Timestamp gpuTime;
    Timestamp fromSubmitToFence;
    vector<GraphNodeTiming> nodes;
  };

  struct SubmitTiming
  {
    uint64_t id;
    int listsCount;
    Timestamp timeBeforeSubmit;
    Timestamp submitCpuTime;
    Timestamp fillCommandLists;
    Timestamp addNodes;
    Timestamp graphSolve;
    Timestamp submitSolve;
    vector<CommandListTiming> lists;
  };
};
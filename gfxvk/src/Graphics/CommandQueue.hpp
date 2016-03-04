#pragma once
#include "CommandList.hpp"
#include "Fence.hpp"
#include "VulkanQueue.hpp"

#include <memory>

// trying out interface approach to simplify backend implementations and remove duplicate code.
using PQueue = VulkanQueue;

// hosts basic queue commands.
class Queue_
{
private:
  friend class test;
  friend class ApiTests;
  friend class GpuDevice;
  friend class _GpuBracket;
  friend class GraphicsQueue;
  friend class ComputeQueue;
  friend class DMAQueue;
  PQueue m_queue;

  Queue_(PQueue queue);

public:
  void insertFence(GpuFence& fence);
  bool isValid();
};

class DMAQueue : public Queue_
{
private:
  friend class test;
  friend class ApiTests;
  friend class GpuDevice;
  friend class _GpuBracket;
  DMAQueue(PQueue queue);
public:
  void submit(DMACmdBuffer& list);
};

class ComputeQueue : public Queue_
{
private:
  friend class test;
  friend class ApiTests;
  friend class GpuDevice;
  friend class _GpuBracket;
  friend class GraphicsQueue;
  ComputeQueue(PQueue queue);
public:
  void submit(ComputeCmdBuffer& list);
};

// Graphics queue accepts also computeCmdBuffers.
class GraphicsQueue : public ComputeQueue
{
private:
  friend class test;
  friend class ApiTests;
  friend class GpuDevice;
  friend class _GpuBracket;
  GraphicsQueue(PQueue queue);
public:
  void submit(GraphicsCmdBuffer& list);
};


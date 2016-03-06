#pragma once
#include "CommandList.hpp"
#include "Fence.hpp"
#include "vk_specifics/VulkanQueue.hpp"

// hosts basic queue commands.
class GpuQueue 
{
private:
  friend class test;
  friend class ApiTests;
  friend class GpuDevice;
  friend class _GpuBracket;
  friend class GraphicsQueue;
  friend class ComputeQueue;
  friend class DMAQueue;
  QueueImpl m_queue;

  GpuQueue(QueueImpl queue);

public:
  void insertFence(GpuFence& fence);
  bool isValid();
};

class DMAQueue : public GpuQueue
{
private:
  friend class test;
  friend class ApiTests;
  friend class GpuDevice;
  friend class _GpuBracket;
  DMAQueue(QueueImpl queue);
public:
  void submit(DMACmdBuffer& list);
};

class ComputeQueue : public GpuQueue
{
private:
  friend class test;
  friend class ApiTests;
  friend class GpuDevice;
  friend class _GpuBracket;
  friend class GraphicsQueue;
  ComputeQueue(QueueImpl queue);
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
  GraphicsQueue(QueueImpl queue);
public:
  void submit(GraphicsCmdBuffer& list);
};


#pragma once
#include "CommandList.hpp"
#include "Fence.hpp"

#include <memory>

// trying out interface approach to simplify backend implementations and remove duplicate code.
class IQueue
{
public:
  virtual bool isValid() = 0;
  virtual void insertFence() = 0;
  virtual void submit(ICmdBuffer& buffer) = 0;
};

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
  std::shared_ptr<IQueue> m_queue;

  Queue_(std::shared_ptr<IQueue> queue);

  IQueue& getRef()
  {
    return *m_queue.get();
  }
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
  DMAQueue(std::shared_ptr<IQueue> queue);
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
  ComputeQueue(std::shared_ptr<IQueue> queue);
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
  GraphicsQueue(std::shared_ptr<IQueue> queue);
public:
  void submit(GraphicsCmdBuffer& list);
};


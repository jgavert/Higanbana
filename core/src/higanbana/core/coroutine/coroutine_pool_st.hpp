#pragma once
#include "higanbana/core/profiling/profiling.hpp"
#include <atomic>
#include <vector>
#include <mutex>
#include <thread>
#include <optional>
#include <experimental/coroutine>


namespace higanbana
{
namespace experimental
{
namespace stts
{
// this is the handle that is co_awaiting a child.
struct StackTask
{
  std::atomic_int* reportCompletion;
  std::experimental::coroutine_handle<> handle;
  std::vector<std::pair<uintptr_t, std::atomic_int*>> childs;
  std::optional<std::pair<uintptr_t, std::atomic_int*>> currentWaitJoin; // handle address that is waited to be complete, so that handle can continue.
};

// spawned when a coroutine is created
struct FreeLoot
{
  std::experimental::coroutine_handle<> handle; // this might spawn childs, becomes host that way.
  std::atomic_int* reportCompletion; // when task is done, inform here
  // I thought of separate queue where to add "completed tasks", but using atomics for icity.
};

struct StealableQueue
{
  std::deque<FreeLoot> queue; // get it if you can :smirk:
  std::mutex lock; // version 1 stealable queue works with mutex.
};
struct ThreadData
{
  std::deque<StackTask> m_unfinishedBusiness; // added to front and front is worked on.
  StealableQueue publicQueue;
};
namespace v2
{
  extern thread_local bool thread_from_pool;
  extern thread_local int thread_id;
}
class SingleThreadPool
{
  // a single variable to sleep on :shock:
  std::mutex sleepLock;
  std::condition_variable cv;
  // there is only single thread so this is simple
  ThreadData data;
  public:

  bool canExecute(StackTask& task) {
    if (task.currentWaitJoin)
      return task.currentWaitJoin.value().second->load() == 0;
    return !task.handle.done();
  }

  bool isDone(StackTask& task) {
    if (!task.handle.done() || task.currentWaitJoin)
      return false;
    for (auto& child : task.childs) {
      if (child.second->load() > 0)
        return false;
    }
    return true;
  }

  std::optional<FreeLoot> stealTask(ThreadData& task) {
    std::unique_lock lock(task.publicQueue.lock);
    if (task.publicQueue.queue.empty())
      return std::optional<FreeLoot>();
    auto freetask = task.publicQueue.queue.front();
    task.publicQueue.queue.pop_front();
    return std::optional<FreeLoot>(freetask);
  }

  std::optional<FreeLoot> unfork(ThreadData& task) {
    std::unique_lock lock(task.publicQueue.lock);
    if (task.publicQueue.queue.empty())
      return std::optional<FreeLoot>();
    auto freetask = task.publicQueue.queue.back();
    task.publicQueue.queue.pop_back();
    return std::optional<FreeLoot>(freetask);
  }

  // coroutine created 
  void spawnTask(std::experimental::coroutine_handle<> handle) {
    FreeLoot loot{};
    loot.handle = handle;
    if (!data.m_unfinishedBusiness.empty())
    {
      std::atomic_int* counter = new std::atomic_int(1);
      loot.reportCompletion = counter;
      data.m_unfinishedBusiness.front().childs.push_back(std::make_pair(reinterpret_cast<uintptr_t>(handle.address()), counter));
    }
    // add task to own queue
    std::unique_lock lock(data.publicQueue.lock);
    data.publicQueue.queue.push_back(std::move(loot));
  }

  // co_await
  void addDependencyToCurrentTask(std::experimental::coroutine_handle<> handle) {
    std::atomic_int* tracker = nullptr;
    uintptr_t addr = reinterpret_cast<uintptr_t>(handle.address());
    for (auto&& it : data.m_unfinishedBusiness.front().childs) {
      if (it.first == addr) {
        tracker = it.second;
        break;
      }
    }
    assert(tracker != nullptr);
    data.m_unfinishedBusiness.front().currentWaitJoin = std::make_pair(reinterpret_cast<uintptr_t>(handle.address()), tracker);
  }

  void execute() {
    while(!data.m_unfinishedBusiness.empty() || !data.publicQueue.queue.empty()) {
      if (!data.m_unfinishedBusiness.empty()) {
        auto& task = data.m_unfinishedBusiness.front();
        if (canExecute(task)) {
          task.currentWaitJoin = {};
          task.handle.resume();
        }
        if (isDone(task)) {
            if (task.reportCompletion)
              task.reportCompletion->store(0);
            for (auto&& it : data.m_unfinishedBusiness.front().childs) {
              delete it.second;
            }
            data.m_unfinishedBusiness.pop_front();
        }
        else {
          if (auto task = unfork(data)) {
            StackTask st{};
            st.reportCompletion = task.value().reportCompletion;
            st.handle = task.value().handle;
            data.m_unfinishedBusiness.push_front(st);
          }
          else {
            assert(false); // kek
          }
        }
      }
      else if (data.m_unfinishedBusiness.empty()) {
        if (auto task = stealTask(data)) {
          StackTask st{};
          st.reportCompletion = task.value().reportCompletion;
          st.handle = task.value().handle;
          data.m_unfinishedBusiness.push_front(st);
        }
      }
    }
  }
};
namespace globals
{
  void createSingleThreadPool();
  extern std::unique_ptr<SingleThreadPool> s_pool;
}
}
}
}
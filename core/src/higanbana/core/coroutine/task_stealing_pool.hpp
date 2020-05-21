#pragma once
#include "higanbana/core/profiling/profiling.hpp"
#include "higanbana/core/thread/cpu_info.hpp"
#include <atomic>
#include <vector>
#include <mutex>
#include <thread>
#include <optional>
#include <thread>
#include <algorithm>
#include <experimental/coroutine>
#include <windows.h>

// stealer always keeps handles alive until it can prove that all dependencies are done...
#define STEALER_DESTROY_HANDLES

// define this to enable atomic stat collection
#define STEALER_COLLECT_STATS

#ifdef STEALER_COLLECT_STATS
#define STEALER_STATS_IF if (1)
#else
#define STEALER_STATS_IF if (0)
#endif


namespace higanbana
{
namespace taskstealer
{
struct StackTask
{
  std::atomic_int* reportCompletion;
  std::experimental::coroutine_handle<> handle;
  std::vector<std::pair<std::experimental::coroutine_handle<>, std::atomic_int*>> childs;
  std::optional<std::pair<std::experimental::coroutine_handle<>, std::atomic_int*>> currentWaitJoin; // handle address that is waited to be complete, so that handle can continue.

  bool canExecute() const noexcept {
    if (handle.done())
      return false;
    if (currentWaitJoin)
      return currentWaitJoin.value().second->load() == 0;
    return true;
  }
  bool done() const noexcept {
    if (currentWaitJoin)
      return false;
    for (auto& child : childs) 
      if (child.second->load() > 0)
        return false;
    return handle.done();
  }
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
  size_t m_group_id = 0;
  std::deque<FreeLoot> loot; // get it if you can :smirk:
  std::mutex lock; // version 1 stealable queue works with mutex.
  StealableQueue(size_t group_id): m_group_id(group_id){}
  StealableQueue(StealableQueue& other): m_group_id(other.m_group_id){}
  StealableQueue(StealableQueue&& other): m_group_id(other.m_group_id){}
};

struct ThreadData
{
  std::deque<StackTask> m_coroStack;
  size_t m_id = 0;
  size_t m_wakeThread = 0;
  size_t m_group_id = 0;
  uint64_t m_group_mask = 0;
  ThreadData(){}
  ThreadData(size_t id, size_t group_id, uint64_t group_mask):m_id(id), m_group_id(group_id), m_group_mask(group_mask){}
  ThreadData(ThreadData& other) : m_coroStack(other.m_coroStack), m_id(other.m_id), m_group_id(other.m_group_id), m_group_mask(other.m_group_mask){}
  ThreadData(ThreadData&& other) : m_coroStack(std::move(other.m_coroStack)), m_id(other.m_id), m_group_id(other.m_group_id), m_group_mask(other.m_group_mask){}
};
namespace locals
{
  extern thread_local bool thread_from_pool;
  extern thread_local int thread_id;
}

struct StealStats
{
  size_t tasks_done = 0;
  size_t tasks_stolen = 0;
  size_t steal_tries = 0;
  size_t tasks_unforked = 0;
  size_t tasks_stolen_within_l3 = 0;
  size_t tasks_stolen_outside_l3 = 0;
};

class TaskStealingPool
{
  // there is only single thread so this is simple
  std::vector<ThreadData> m_data;
  std::vector<StealableQueue> m_stealQueues; // separate to avoid false sharing
  size_t m_threads = 0;
  std::atomic_size_t m_globalTasksLeft = 0;
  std::atomic_size_t m_doable_tasks = 0;
  std::atomic_size_t m_thread_sleeping = 0;

  // hmm, to make threads sleep in groups...? No sense to wake threads from outside L3 if L3 isn't awake, perf--
  std::mutex sleepLock;
  std::condition_variable cv;

  std::mutex m_global;
  std::vector<FreeLoot> m_nobodyOwnsTasks; // "global tasks", tasks that "main thread" owns.

  std::atomic_bool m_poolAlive;
  std::vector<std::thread> m_threadHandles;
  // statistics
  std::atomic_size_t m_tasks_done = 0;
  std::atomic_size_t m_tasks_stolen_within_l3 = 0;
  std::atomic_size_t m_tasks_stolen_outside_l3 = 0;
  std::atomic_size_t m_steal_fails = 0;
  std::atomic_size_t m_tasks_unforked = 0;
  public:
  StealStats stats() {
    auto stolen = m_tasks_stolen_within_l3 + m_tasks_stolen_outside_l3;
    return {m_tasks_done, stolen, m_steal_fails, m_tasks_unforked, m_tasks_stolen_within_l3, m_tasks_stolen_outside_l3};
  }

  TaskStealingPool() noexcept {
    m_threads = std::thread::hardware_concurrency();
    SystemCpuInfo info;
    size_t l3threads = info.numas.front().threads / info.numas.front().coreGroups.size();

    m_poolAlive = true;
    for (size_t group = 0; group < info.numas.front().coreGroups.size(); ++group){
      for (size_t t = 0; t < l3threads; t++) {
        auto index = group*l3threads + t;
        m_data.emplace_back(index, group, info.numas.front().coreGroups[group].mask);
        m_stealQueues.emplace_back(group);
      }
    }
    m_thread_sleeping = m_threads-1;
    for (size_t t = 1; t < m_threads; t++) {
      m_threadHandles.push_back(std::thread(&TaskStealingPool::thread_loop, this, std::ref(m_data[t])));
      SetThreadAffinityMask(m_threadHandles.back().native_handle(), m_data[t].m_group_mask);
    }
  }
  ~TaskStealingPool() noexcept {
    m_poolAlive = false;
    m_doable_tasks = m_threads+1;
    cv.notify_all();
    for (auto& it : m_threadHandles)
      it.join();
  }

  void wakeThread(ThreadData& thread) noexcept {
    size_t countToWake = std::max(std::min(0ull, m_doable_tasks.load()), m_thread_sleeping.load());
    if (countToWake < 2)
      cv.notify_one();
    else
      cv.notify_all();
  }

  std::optional<FreeLoot> stealTask(const ThreadData& thread) noexcept {
    if (m_doable_tasks > 0)
    {
      for (size_t index = thread.m_id; index < thread.m_id + m_threads; index++)
      {
        auto& ownQueue = m_stealQueues[index % m_threads];
        if (ownQueue.m_group_id != thread.m_group_id)
          continue;
        std::unique_lock lock(ownQueue.lock);
        if (!ownQueue.loot.empty()) {
          auto freetask = ownQueue.loot.front();
          ownQueue.loot.pop_front();
          STEALER_STATS_IF m_tasks_stolen_within_l3++;
          return std::optional<FreeLoot>(freetask);
        }
      }
      for (size_t index = thread.m_id; index < thread.m_id + m_threads; index++)
      {
        auto& ownQueue = m_stealQueues[index % m_threads];
        if (ownQueue.m_group_id == thread.m_group_id)
          continue;
        std::unique_lock lock(ownQueue.lock);
        if (!ownQueue.loot.empty()) {
          auto freetask = ownQueue.loot.front();
          ownQueue.loot.pop_front();
          STEALER_STATS_IF m_tasks_stolen_outside_l3++;
          return std::optional<FreeLoot>(freetask);
        }
      }
    }
    STEALER_STATS_IF m_steal_fails++;
    return std::optional<FreeLoot>();
  }

  std::optional<FreeLoot> unfork(ThreadData& thread) noexcept {
    auto& myStealQueue = m_stealQueues[thread.m_id];
    std::unique_lock lock(myStealQueue.lock);
    if (myStealQueue.loot.empty())
      return std::optional<FreeLoot>();
    auto freetask = myStealQueue.loot.back();
    myStealQueue.loot.pop_back();
    STEALER_STATS_IF m_tasks_unforked++;
    return std::optional<FreeLoot>(freetask);
  }

  // called by coroutine - from constructor 
  [[nodiscard]] uintptr_t spawnTask(std::experimental::coroutine_handle<> handle) noexcept {
    size_t threadID = static_cast<size_t>(locals::thread_id);
    if (!locals::thread_from_pool)
      threadID = 0;
    auto& data = m_data[threadID];
    FreeLoot loot{};
    loot.handle = handle;
    std::atomic_int* counter = new std::atomic_int(1);
    loot.reportCompletion = counter;
    if (!data.m_coroStack.empty())
    {
      data.m_coroStack.front().childs.push_back(std::make_pair(handle, counter));
    }
    else
    {
      // add to global pool for being able to track the source coroutine completion.
      std::lock_guard<std::mutex> guard(m_global);
      m_nobodyOwnsTasks.push_back(loot);
      m_globalTasksLeft++;
    }
    // add task to own queue
    {
      auto& stealQueue = m_stealQueues[threadID];
      std::unique_lock lock(stealQueue.lock);
      stealQueue.loot.push_back(std::move(loot));
    }
    m_doable_tasks++;
    wakeThread(data);
    return reinterpret_cast<uintptr_t>(counter);
  }

  // called by coroutine - when entering co_await, handle is what current coroutine is depending from.
  void addDependencyToCurrentTask(std::experimental::coroutine_handle<> handleSuspending, std::experimental::coroutine_handle<> handleNeeded, uintptr_t trackerPtr) noexcept {
    size_t threadID = static_cast<size_t>(locals::thread_id);
    if (!locals::thread_from_pool)
      threadID = 0;
    auto& data = m_data[threadID];

    assert(!data.m_coroStack.front().currentWaitJoin); // overriding...
    HIGAN_ASSERT(trackerPtr != 0, "tracker should be always valid");
    std::atomic_int* tracker = reinterpret_cast<std::atomic_int*>(trackerPtr);
    assert(data.m_coroStack.front().handle == handleSuspending);
    data.m_coroStack.front().currentWaitJoin = std::make_pair(handleNeeded, tracker);
  }

  void workOnTasks(ThreadData& myData, StealableQueue& myQueue) noexcept {
    if (!myData.m_coroStack.empty()) {
      auto& task = myData.m_coroStack.front();
      if (task.canExecute() && !task.done()) {
        task.currentWaitJoin = {};
        task.handle.resume();
      }
      if (task.done()) {
          auto* ptr = task.reportCompletion;
          for (auto&& it : myData.m_coroStack.front().childs) {
#if defined(STEALER_DESTROY_HANDLES)
            it.first.destroy();
#endif
            delete it.second;
          }
          myData.m_coroStack.pop_front();
          ptr->store(0);
          STEALER_STATS_IF m_tasks_done++;
      }
      else {
        if (auto task = unfork(myData)) {
          StackTask st{};
          st.reportCompletion = task.value().reportCompletion;
          st.handle = task.value().handle;
          myData.m_coroStack.push_front(st);
          m_doable_tasks--;
        }
      }
    }
    else if (myData.m_coroStack.empty()) {
      if (auto task = stealTask(myData)) {
        StackTask st{};
        st.reportCompletion = task.value().reportCompletion;
        st.handle = task.value().handle;
        myData.m_coroStack.push_front(st);
        m_doable_tasks--;
      }
    }
  }

  void thread_loop(ThreadData& myData) noexcept {
    //auto& myData = m_data[threadIndex];
    locals::thread_id = myData.m_id;
    locals::thread_from_pool = true;
    m_thread_sleeping--;
    auto& myQueue = m_stealQueues[myData.m_id];
    while(m_poolAlive){
      if (auto task = stealTask(myData)) {
        StackTask st{};
        st.reportCompletion = task.value().reportCompletion;
        st.handle = task.value().handle;
        myData.m_coroStack.push_front(st);
        m_doable_tasks--;
      } else if (myData.m_coroStack.empty() && m_doable_tasks.load() == 0){
        std::unique_lock<std::mutex> lk(sleepLock);
        m_thread_sleeping++;
        cv.wait(lk, [&](){
          return m_doable_tasks.load() > 0;
        });
        m_thread_sleeping--;
      }
      while(!myData.m_coroStack.empty() || !myQueue.loot.empty())
        workOnTasks(myData, myQueue);
    }

    locals::thread_id = -1;
    locals::thread_from_pool = false;
    m_thread_sleeping++;
  }

  std::atomic_int* findWorkToWaitFor() noexcept {
    std::atomic_int* ptr = nullptr;
    {
      std::lock_guard<std::mutex> guard(m_global);
      if (!m_nobodyOwnsTasks.empty())
        ptr = m_nobodyOwnsTasks.back().reportCompletion;
    }
    return ptr;
  }

  void freeCompletedWork() noexcept {
    std::lock_guard<std::mutex> guard(m_global);
    while(!m_nobodyOwnsTasks.empty() && m_nobodyOwnsTasks.back().reportCompletion->load() == 0) {
      // illegal to destroy handles here, otherwise we cannot get the value from within the handle to the caller.
      delete m_nobodyOwnsTasks.back().reportCompletion;
      m_nobodyOwnsTasks.pop_back();
      m_globalTasksLeft--;
    }
  }

  void execute() noexcept {
    auto& myData = m_data[0];
    auto& myQueue = m_stealQueues[0];
    while(m_globalTasksLeft > 0) {
      std::atomic_int* wait = findWorkToWaitFor();
      while (wait && wait->load() > 0) {
        workOnTasks(myData, myQueue);
      }
      freeCompletedWork();
    }
  }
};
namespace globals
{
  void createTaskStealingPool();
  extern std::unique_ptr<TaskStealingPool> s_stealPool;
}
}
}
#pragma once
#include "higanbana/core/profiling/profiling.hpp"
#include <atomic>
#include <vector>
#include <mutex>
#include <thread>

namespace higanbana
{
namespace experimental
{
class Barrier
{
  friend class BarrierObserver;
  std::shared_ptr<std::atomic<int64_t>> m_counter;
  public:
  Barrier(int taskid = 0)
  {
    std::atomic_store(&m_counter, std::make_shared<std::atomic<int64_t>>(1));
  }
  Barrier(const Barrier& copy) noexcept{
    if (copy.m_counter) {
      std::atomic_store(&m_counter, std::atomic_load(&copy.m_counter));
      auto al = std::atomic_load(&m_counter);
      if (al)
        al->fetch_add(1, std::memory_order_relaxed);
    }
    else {
      std::atomic_store(&m_counter, std::make_shared<std::atomic<int64_t>>(1));
    }
  }
  Barrier(Barrier&& other) noexcept
  {
    std::atomic_store(&m_counter, std::atomic_load(&other.m_counter));
    std::atomic_store(&other.m_counter, std::shared_ptr<std::atomic<int64_t>>(nullptr));
  }
  Barrier& operator=(const Barrier& copy) noexcept{
    if (m_counter)
      std::atomic_load(&m_counter)->fetch_sub(1);
    if (copy.m_counter) {
      std::atomic_store(&m_counter, std::atomic_load(&copy.m_counter));
      auto al = std::atomic_load(&m_counter);
      if (al)
        al->fetch_add(1, std::memory_order_relaxed);
    }
    else {
      std::atomic_store(&m_counter, std::make_shared<std::atomic<int64_t>>(1));
    }
    return *this;
  }
  Barrier& operator=(Barrier&& other) noexcept {
    if (m_counter)
      std::atomic_load(&m_counter)->fetch_sub(1, std::memory_order_relaxed);
    std::atomic_store(&m_counter, std::atomic_load(&other.m_counter));
    std::atomic_store(&other.m_counter, std::shared_ptr<std::atomic<int64_t>>(nullptr));
    return *this;
  }
  bool done() const noexcept {
    return std::atomic_load(&m_counter)->load(std::memory_order_relaxed) <= 0;
  }
  void kill() noexcept {
    if (m_counter) {
      auto local = std::atomic_load(&m_counter);
      if (local->load(std::memory_order_relaxed) > 0)
        local->store(-100, std::memory_order_relaxed);
    }
  }
  explicit operator bool() const noexcept {
    return bool(std::atomic_load(&m_counter));
  }
  ~Barrier() noexcept{
    auto local = std::atomic_load(&m_counter);
    if (local)
      local->fetch_sub(1, std::memory_order_relaxed);
    std::atomic_store(&m_counter, std::shared_ptr<std::atomic<int64_t>>(nullptr));
  }
};

class BarrierObserver 
{
  std::shared_ptr<std::atomic<int64_t>> m_counter;
  public:
  BarrierObserver() noexcept
  {
    std::atomic_store(&m_counter, std::make_shared<std::atomic<int64_t>>(0));
  }
  BarrierObserver(const Barrier& barrier) noexcept
  {
    std::atomic_store(&m_counter, std::atomic_load(&barrier.m_counter));
  }
  BarrierObserver(const BarrierObserver& copy) noexcept
  {
    std::atomic_store(&m_counter, std::atomic_load(&copy.m_counter));
  }
  BarrierObserver(BarrierObserver&& other) noexcept
  {
    std::atomic_store(&m_counter, std::atomic_load(&other.m_counter));
    std::atomic_store(&other.m_counter, std::shared_ptr<std::atomic<int64_t>>(nullptr));
  }
  BarrierObserver& operator=(const BarrierObserver& copy) noexcept{
    std::atomic_store(&m_counter, std::atomic_load(&copy.m_counter));
    return *this;
  }
  BarrierObserver& operator=(BarrierObserver&& other) noexcept {
    std::atomic_store(&m_counter, std::atomic_load(&other.m_counter));
    std::atomic_store(&other.m_counter, std::shared_ptr<std::atomic<int64_t>>(nullptr));
    return *this;
  }
  Barrier barrier() noexcept{
    Barrier bar;
    auto local = std::atomic_load(&m_counter);
    local->fetch_add(1);
    std::atomic_store(&bar.m_counter, local);
    return bar;
  }
  bool done() const noexcept {
    if (!m_counter)
      return true;
    return std::atomic_load(&m_counter)->load() <= 0;
  }
  explicit operator bool() const noexcept{
    return bool(std::atomic_load(&m_counter));
  }
  ~BarrierObserver() noexcept{
  } 
};

class Task
{
public:
  Task()  :
    m_id(0),
    m_iterations(0),
    m_iterID(0),
    m_ppt(1),
    m_sharedWorkCounter(nullptr)
  {
  };
  Task(size_t id, size_t start, size_t iterations, Barrier barrier) :
    m_id(id),
    m_iterations(iterations),
    m_iterID(start),
    m_ppt(1),
    m_sharedWorkCounter(iterations == 1? nullptr : new std::atomic<size_t>(iterations)),
    m_blocks(std::move(barrier))
  {
  };

private:
  Task(size_t id, size_t start, size_t iterations, std::atomic<size_t>* sharedWorkCount, Barrier barrier) :
    m_id(id),
    m_iterations(iterations),
    m_iterID(start),
    m_ppt(1),
    m_sharedWorkCounter(sharedWorkCount),
    m_blocks(std::move(barrier))
  {
    HIGAN_ASSERT(sharedWorkCount != nullptr, "shared work count should exist.");
  };
public:

  size_t m_id;
  size_t m_iterations;
  size_t m_iterID;
  std::atomic<size_t>* m_sharedWorkCounter;
  Barrier m_blocks; // maybe there is always one?...?

  std::function<bool(size_t&, size_t&)> f_work;
  int m_ppt;

  // Generates ppt sized for -loop lambda inside this work.
  template<size_t ppt, typename Func>
  void genWorkFunc(Func&& func) noexcept
  {
    f_work = [func](size_t& iterID, size_t& iterations) mutable -> bool
    {
      assert(iterations > 0);
      if (iterations == 0)
      {
        return true;
      }
      if (ppt > iterations)
      {
        for (size_t i = 0; i < iterations; ++i)
        {
          func(iterID);
          ++iterID;
        }
        iterations = 0;
      }
      else
      {
        for (size_t i = 0; i < ppt; ++i)
        {
          func(iterID);
          ++iterID;
        }
        iterations -= ppt;
      }
      return iterations == 0;
    };
    m_ppt = ppt;
  }

  inline bool doWork() noexcept
  {
    __assume(f_work);
    return f_work(m_iterID, m_iterations);
  }

  inline bool canSplit() noexcept
  {
    return (m_iterations > static_cast<size_t>(m_ppt));
  }

  inline Task split() noexcept
  {
    auto iters = m_iterations / 2;
    auto newStart = m_iterID + iters;
    Task splittedWork(m_id, newStart, iters + m_iterations % 2, m_sharedWorkCounter, m_blocks);
    splittedWork.f_work = f_work;
    splittedWork.m_ppt = m_ppt;
    m_iterations = iters;
    assert(splittedWork.m_iterations != 0);
    return splittedWork;
  }
};

// hosts all per thread data, the local work queue and current task.
class ThreadData
{
public:
  ThreadData() : m_task(Task()), m_ID(0) { }
  ThreadData(int id) : m_task(Task()), m_ID(id) {  }

  ThreadData(const ThreadData& data) : m_task(Task()), m_ID(data.m_ID) {}
  ThreadData(ThreadData&& data) : m_task(Task()), m_ID(data.m_ID) {}

  Task m_task;
  std::atomic<int> m_localQueueSize = {0};
  std::deque<Task> m_localDeque;
  std::atomic<bool> m_alive = {true};
  bool m_failedToCheckGlobalQueue = false;
  int m_ID = 0;
};



#define LBSPOOL_ENABLE_PROFILE_THREADS
// hosts the worker threads for lbs_awaitable and works as the backend
namespace internal
{
extern thread_local bool thread_from_pool;
extern thread_local int thread_id;
}
class LBSPool
{
  struct AllThreadData 
  {
    //public:
    AllThreadData(int i):data(i), mutex(std::make_unique<std::mutex>()), cv(std::make_unique<std::condition_variable>()) {}
    //AllThreadData(const AllThreadData& other):data(i), mutex(std::make_unique<std::mutex>()), cv(std::make_unique<std::condition_variable>()) {}
    //AllThreadData(AllThreadData&& other):data(i), mutex(std::make_unique<std::mutex>()), cv(std::make_unique<std::condition_variable>()) {}
    ThreadData data;
    std::unique_ptr<std::mutex> mutex;
    std::vector<Task> addableTasks;
    std::unique_ptr<std::condition_variable> cv;
  };
  std::vector<std::thread> m_threads;
  std::vector<AllThreadData>  m_threadData;
  std::atomic<size_t>      m_nextTaskID; // just increasing index...
  std::atomic<bool> StopCondition;
  std::atomic<int> idle_threads;
  std::atomic<int> threads_awake;
  std::atomic<int> tasks_to_do;
  std::atomic<int> tasks_in_queue;
  std::atomic<size_t> tasks_done;

  std::mutex m_globalWorkMutex;
  std::vector<std::pair<std::vector<BarrierObserver>, Task>> m_taskQueue;

  void notifyAll(int whoNotified, bool ignoreTasks = false) noexcept
  {
    if (ignoreTasks)
    {
      auto offset = static_cast<unsigned>(std::max(0, whoNotified));
      const unsigned tdSize = static_cast<unsigned>(m_threadData.size());
      for (unsigned i = 1; i < tdSize; ++i) 
      {
        auto& it = m_threadData[(i+offset)%tdSize];
        if (it.data.m_ID != offset)
        {
          it.cv->notify_one();
        }
      }
      return;
    }

    auto doableTasks = tasks_to_do.load(std::memory_order::relaxed); 
    // 1 so that there is a task for this thread, +1 since it's likely that one thread will end up without a task. == 2
    unsigned offset = static_cast<unsigned>(std::max(0, whoNotified)+1);
    const unsigned tdSize = std::max(1u, static_cast<unsigned>(m_threadData.size()));
    auto tasks = std::min(static_cast<int>(tdSize)-1, std::max(1, doableTasks - 2)); // slightly modify how many threads to actually wake up, 2 is deemed as good value
    
    constexpr int breakInPieces = 1;
    const unsigned innerLoop = tdSize-1;
    if (tasks <= 0)
      return;
    for (unsigned i = 0; i < innerLoop; ++i) 
    {
      if (tasks <= 0)
        break;
      auto& it = m_threadData[(offset+i)%tdSize];
      tasks--;
      it.cv->notify_one();
    }
  }

  public:
  int tasksAdded() noexcept {return tasks_in_queue.load(std::memory_order::relaxed);}
  int tasksReadyToProcess() noexcept {return tasks_to_do.load(std::memory_order::relaxed);}
  int my_queue_count() noexcept {
    if (internal::thread_from_pool)
      return m_threadData[internal::thread_id].data.m_localQueueSize.load(std::memory_order::relaxed);
    return 0;
  }
  bool am_i_from_pool() noexcept {
    return internal::thread_from_pool;
  }
  int my_thread_index() noexcept {
    if (internal::thread_from_pool)
      return internal::thread_id;
    return 0;
  }

  void launchThreads() noexcept {
    
  }

  LBSPool()
    : m_nextTaskID(1) // keep 0 as special task index.
    , StopCondition(false)
    , idle_threads(0)
    , threads_awake(0)
    , tasks_to_do(0)
    , tasks_in_queue(0)
    , tasks_done(0)
  {
    HIGAN_CPU_FUNCTION_SCOPE();
    unsigned procs = std::thread::hardware_concurrency();
    for (unsigned i = 0; i < procs; i++)
    {
      m_threadData.emplace_back(i);
    }
    assert(!m_threadData.empty() && m_threadData.size() == procs);
    for (auto&& it : m_threadData)
    {
      if (it.data.m_ID == 0) // Basically mainthread is one of *us*
        continue; 
      m_threads.push_back(std::thread(&LBSPool::loop, this, it.data.m_ID));
    }
    HIGAN_CPU_BRACKET("waiting for threads to wakeup for instant business.");
    while(threads_awake != procs-1);
  }
  void resetIDs() noexcept { m_nextTaskID = 1;}
  LBSPool(const LBSPool& asd) = delete;
  LBSPool(LBSPool&& asd) = delete;
  LBSPool& operator=(const LBSPool& asd) = delete;
  LBSPool& operator=(LBSPool&& asd) = delete;
  ~LBSPool() noexcept  // you do not simply delete this
  {
    HIGAN_CPU_FUNCTION_SCOPE();
    StopCondition.store(true);
    tasks_to_do = 32;
    for (auto&& data : m_threadData) {
      if (data.data.m_ID !=0)
        while (data.data.m_alive.load())
          notifyAll(-1, true);
    }
    for (auto& it : m_threads)
    {
      it.join();
    }
  }
  void loop(int i) noexcept
  {
    internal::thread_from_pool = true;
    internal::thread_id = i;
    threads_awake++;
    std::string threadName = "Thread ";
    threadName += std::to_string(i);
    threadName += '\0';
    AllThreadData& p = m_threadData.at(i);
    {
      stealOrWait(p);
    }
    while (!StopCondition) {
      if (p.data.m_task.canSplit() && tasks_to_do < 128) {
        if (p.data.m_localQueueSize.load(std::memory_order::relaxed) == 0) {
          {
            std::lock_guard<std::mutex> guard(*p.mutex);
            p.data.m_localDeque.emplace_back(std::move(p.data.m_task.split())); // push back
            p.data.m_localQueueSize.store(p.data.m_localDeque.size(), std::memory_order_relaxed);
          }
          tasks_to_do++;
          notifyAll(p.data.m_ID);
          continue;
        }
      }
      if (doWork(p))
      {
        stealOrWait(p);
      }
    }
    p.data.m_alive.store(false);
    threads_awake--;
    internal::thread_from_pool = false;
    internal::thread_id = 0;
  }

  inline bool stealOrSleep(AllThreadData& data, bool allowedToSleep = true) noexcept {
    //HIGAN_CPU_FUNCTION_SCOPE();
    // check my own deque
    auto& p = data.data;
    if (p.m_localQueueSize.load(std::memory_order::relaxed) > 0)
    {
      // try to take work from own deque, backside.
      std::lock_guard<std::mutex> guard(*data.mutex);
      if (!p.m_localDeque.empty())
      {
        p.m_task = std::move(p.m_localDeque.front());
        p.m_localDeque.pop_front();
        p.m_localQueueSize.store(p.m_localDeque.size(), std::memory_order_relaxed);
        tasks_to_do.fetch_sub(1, std::memory_order::relaxed);
        return true;
      }
    }
    // other deques
    const unsigned tdSize = static_cast<int>(m_threadData.size());
    for (unsigned i = 0; i < tdSize; ++i) 
    {
      auto threadId = (i+p.m_ID) % tdSize;
      if (threadId == p.m_ID)
        continue;
      auto& it = m_threadData[threadId];
      if (it.data.m_localQueueSize.load(std::memory_order::relaxed) > 0) // this should reduce unnecessary lock_guards, and cheap.
      {
        std::unique_lock<std::mutex> guard(*it.mutex);
        if (!it.data.m_localDeque.empty()) // double check as it might be empty now.
        {
          p.m_task = it.data.m_localDeque.back();
          assert(p.m_task.m_iterations != 0);
          it.data.m_localDeque.pop_back();
          it.data.m_localQueueSize.store(it.data.m_localDeque.size(), std::memory_order::relaxed);
          tasks_to_do.fetch_sub(1, std::memory_order::relaxed);
          return true;
        }
      }
    }
    // as last effort, check global queue
    if (tryTakeTaskFromGlobalQueue(data)) {
      return true;
    }

    // if all else fails, wait for more work.
    if (!StopCondition && allowedToSleep && !data.data.m_failedToCheckGlobalQueue) // this probably doesn't fix the random deadlock
    {
      if (p.m_localQueueSize.load(std::memory_order::relaxed) != 0 || tasks_to_do.load(std::memory_order::relaxed) > 0)
      {
        return false;
      }
      std::unique_lock<std::mutex> lk(*data.mutex);
      idle_threads.fetch_add(1, std::memory_order::relaxed);
      data.cv->wait(lk, [&]{
        if (tasks_to_do.load(std::memory_order::relaxed) > 0)
          return true;
        return false;
      }); // thread sleep
      idle_threads.fetch_sub(1, std::memory_order::relaxed);
    }
    return false;
  }

  inline void stealOrWait(AllThreadData& data, bool allowedToSleep = true) noexcept
  {
    ThreadData& p = data.data;
    p.m_task.m_id = 0;
    while (0 == p.m_task.m_id && !StopCondition)
    {
      stealOrSleep(data, allowedToSleep);
    }
  }

  bool tryTakeTaskFromGlobalQueue(AllThreadData& data) noexcept
  {
    std::vector<Task>& addable = data.addableTasks;
    if (tasks_in_queue.load(std::memory_order::relaxed) > 0)
    {
      if (!addable.empty())
        addable.clear();
      std::unique_lock<std::mutex> guard(m_globalWorkMutex, std::defer_lock_t());
      data.data.m_failedToCheckGlobalQueue = false;
      if (guard.try_lock()) {
        m_taskQueue.erase(std::remove_if(m_taskQueue.begin(), m_taskQueue.end(), [&](const std::pair<std::vector<BarrierObserver>, Task>& it){
          if (allDepsDone(it.first)) {
            addable.push_back(std::move(it.second));
            return true;
          }
          return false;
        }), m_taskQueue.end());
      } else {
        data.data.m_failedToCheckGlobalQueue = true;
      }
      tasks_in_queue -= static_cast<int>(addable.size());
    }
    if (!addable.empty())
    {
      std::lock_guard<std::mutex> guard(*data.mutex);
      data.data.m_localDeque.insert(data.data.m_localDeque.end(), addable.begin(), addable.end());
      data.data.m_localQueueSize.store(data.data.m_localDeque.size(), std::memory_order_relaxed);
      tasks_to_do += static_cast<int>(addable.size());
      notifyAll(data.data.m_ID);
      addable.clear();
      return true;
    }
    return false;
  }

  void informTaskFinished(AllThreadData& data) noexcept
  {
#if defined(LBSPOOL_ENABLE_PROFILE_THREADS)
    HIGAN_CPU_FUNCTION_SCOPE();
#endif
    data.data.m_task.m_id = 0;
    //BarrierObserver observs = data.data.m_task.m_blocks;
    if (data.data.m_task.m_sharedWorkCounter)
      delete data.data.m_task.m_sharedWorkCounter;
    data.data.m_task.m_blocks.kill();
    //data.data.m_task.m_blocks = Barrier();
    tryTakeTaskFromGlobalQueue(data);
  }

  inline void didWorkFor(AllThreadData& data, size_t amount) noexcept // Task specific counter this time
  {
#if defined(LBSPOOL_ENABLE_PROFILE_THREADS)
    HIGAN_CPU_FUNCTION_SCOPE();
#endif
    if (amount <= 0)
    {
      assert(false);
      return;
    }
    if (data.data.m_task.m_sharedWorkCounter == nullptr || data.data.m_task.m_sharedWorkCounter->fetch_sub(amount) - amount <= 0) // be careful with the fetch_sub command
    {
      informTaskFinished(data);
    }
    if (data.data.m_task.m_id == 0 || data.data.m_task.m_iterations == 0){
      data.data.m_task = Task();
      tasks_done++;
    }
  }

  inline bool doWork(AllThreadData& data) noexcept
  {
#if defined(LBSPOOL_ENABLE_PROFILE_THREADS)
    HIGAN_CPU_FUNCTION_SCOPE();
#endif
    ThreadData& p = data.data;
    auto currentIterID = p.m_task.m_iterID;
    bool rdy = false;
    rdy = p.m_task.doWork();
    auto amountOfWork = p.m_task.m_iterID - currentIterID;
    didWorkFor(data, amountOfWork);
    return rdy;
  }

  inline bool allDepsDone(const std::vector<BarrierObserver>& barrs) noexcept {
    for (const auto& barrier : barrs) {
      if (!barrier.done())
        return false;
    }
    return true;
  }

  bool wantToAddTask() noexcept {
    int queue_count =  my_queue_count();
    bool spawnTask = !(queue_count > 4);
    return spawnTask || true;
  }
/*
  bool wantToAddTaskWithDeps(std::initializer_list<BarrierObserver>&& depends) noexcept {
    if (wantToAddTask())
      return true;
    return !allDepsDone(std::forward<decltype(depends)>(depends));
  }*/

  template<typename Func>
  inline BarrierObserver task(std::vector<BarrierObserver>&& depends, Func&& func) noexcept {
    return internalAddTask<1>(std::forward<decltype(depends)>(depends), 0, 1, std::forward<Func>(func));
  }

  template<size_t ppt, typename Func>
  inline BarrierObserver internalAddTask(std::vector<BarrierObserver>&& depends, size_t start_iter, size_t iterations, Func&& func) noexcept
  {
#if defined(LBSPOOL_ENABLE_PROFILE_THREADS)
    HIGAN_CPU_FUNCTION_SCOPE();
#endif
    int ThreadID = my_thread_index();
    size_t newId = m_nextTaskID.fetch_add(1);
    assert(newId < m_nextTaskID);
    BarrierObserver obs;
    Task newTask(newId, start_iter, iterations, std::move(obs.barrier()));
    auto& threadData = m_threadData.at(ThreadID);
    newTask.genWorkFunc<ppt>(std::forward<Func>(func));
    {
      if (depends.empty() || allDepsDone(depends))
      {
        std::unique_lock<std::mutex> u2(*threadData.mutex);
        threadData.data.m_localDeque.push_back(std::move(newTask));
        threadData.data.m_localQueueSize.store(threadData.data.m_localDeque.size());
        tasks_to_do++;
      }
      else
      {
        std::unique_lock<std::mutex> u1(m_globalWorkMutex);
        m_taskQueue.push_back({ std::move(depends), std::move(newTask) });
        tasks_in_queue++;
      }
    }
    notifyAll(-1);
    return obs;
  }

  void helpTasksUntilBarrierComplete(BarrierObserver observed) noexcept {
    assert(my_thread_index() == 0);
#if defined(LBSPOOL_ENABLE_PROFILE_THREADS)
    HIGAN_CPU_FUNCTION_SCOPE();
#endif
    auto& p = m_threadData.at(0);
    while (!observed.done())
    {
      if (p.data.m_task.m_id == 0)
        stealOrSleep(p, false);
      if (p.data.m_task.m_id != 0) {
        // can we split work?
        if (p.data.m_task.canSplit())
        {
          if (p.data.m_localQueueSize.load() == 0)
          { // Queue didn't have anything, adding.
            {
              std::lock_guard<std::mutex> guard(*p.mutex);
              p.data.m_localDeque.push_back(p.data.m_task.split()); // push back
              p.data.m_localQueueSize.store(p.data.m_localDeque.size());
            }
            notifyAll(p.data.m_ID);
            continue;
          }
        }
        // we couldn't split or queue had something
        doWork(p);
      }
    }
  }

  size_t tasksDone() const noexcept { return tasks_done;}
};
namespace globals 
{
  void createGlobalLBSPool();
  extern std::unique_ptr<LBSPool> s_pool;
  extern thread_local bool thread_from_pool;
  extern thread_local int thread_id;
}
}
}
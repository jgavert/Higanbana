#include <catch2/catch.hpp>

#include <higanbana/core/profiling/profiling.hpp>

#include <vector>
#include <thread>
#include <future>
#include <optional>
#include <cstdio>
#include <iostream>
#include <deque>
#include <experimental/coroutine>
#include <windows.h>


struct suspend_never {
  constexpr bool await_ready() const noexcept { return true; }
  void await_suspend(std::experimental::coroutine_handle<>) const noexcept {}
  constexpr void await_resume() const noexcept {}
};

struct suspend_always {
  constexpr bool await_ready() const noexcept { return false; }
  void await_suspend(std::experimental::coroutine_handle<>) const noexcept {}
  constexpr void await_resume() const noexcept {}
};
template<typename T>
class async_awaitable {
public:
  struct promise_type {
    using coro_handle = std::experimental::coroutine_handle<promise_type>;
    auto get_return_object() {
      //printf("get_return_object\n");
      return coro_handle::from_promise(*this);
    }
    auto initial_suspend() {
      //-printf("initial_suspend\n");
      return suspend_always();
    }
    auto final_suspend() noexcept {
      //printf("final_suspend\n");
      return suspend_always();
    }
    void return_value(T value) {m_value = value;}
    void unhandled_exception() {
      std::terminate();
    }
    auto await_transform(async_awaitable<T> handle) {
      //printf("await_transform\n");
      return handle;
    }
    T m_value;
    std::future<void> m_fut;
    std::promise<void> finalFut;
    std::future<void> m_final;
    std::weak_ptr<coro_handle> weakref;
  };
  using coro_handle = std::experimental::coroutine_handle<promise_type>;
  async_awaitable(coro_handle handle) : handle_(std::shared_ptr<coro_handle>(new coro_handle(handle), [](coro_handle* ptr){ptr->destroy(); delete ptr;}))
  {
    //printf("created async_awaitable\n");
    assert(handle);
    handle_->promise().m_final = handle_->promise().finalFut.get_future();

    handle_->promise().weakref = handle_;
    handle_->promise().m_fut = std::async(std::launch::async, [handlePtr = handle_]
    {
      if (!handlePtr->done()) {
        handlePtr->resume();
        if (handlePtr->done())
          handlePtr->promise().finalFut.set_value();
      }
    });
  }
  async_awaitable(async_awaitable& other) {
    handle_ = other.handle_;
  };
  async_awaitable(async_awaitable&& other) : handle_(std::move(other.handle_)) {
    //printf("moving\n");
    assert(handle_);
    other.handle_ = nullptr;
    //other.m_fut = nullptr;
  }
  // coroutine meat
  T await_resume() noexcept {
    return handle_->promise().m_value;
  }
  bool await_ready() noexcept {
    return handle_->done();
  }

  // enemy coroutine needs this coroutines result, therefore we compute it.
  void await_suspend(coro_handle handle) noexcept {
    handle_->promise().m_fut.wait();
    handle_->promise().m_final.wait();
    //handle.promise().m_fut.wait();
    std::shared_ptr<coro_handle> otherHandle = handle.promise().weakref.lock();
    
    
    auto newfut = std::async(std::launch::async, [handlePtr = otherHandle]() mutable
    {
      if (!handlePtr->done()) {
        handlePtr->resume();
        if (handlePtr->done())
          handlePtr->promise().finalFut.set_value();
      }
    });
    handle.promise().m_fut = std::move(newfut);
  }
  // :o
  ~async_awaitable() { }
  T get()
  {
    //printf("get\n");
    handle_->promise().m_final.wait();
    assert(handle_->done());
    return handle_->promise().m_value; 
  }
private:
  std::shared_ptr<coro_handle> handle_;
};
class Barrier
{
  friend class BarrierObserver;
  std::shared_ptr<std::atomic<int64_t>> m_counter;
  public:
  Barrier(int taskid = 0)
  {
    std::atomic_store(&m_counter, std::make_shared<std::atomic<int64_t>>(1));
  }
  Barrier(const Barrier& copy){
    if (copy.m_counter) {
      std::atomic_store(&m_counter, std::atomic_load(&copy.m_counter));
      auto al = std::atomic_load(&m_counter);
      if (al)
        al->fetch_add(1);
    }
    else {
      std::atomic_store(&m_counter, std::make_shared<std::atomic<int64_t>>(1));
    }
  }
  Barrier(Barrier&& other)
  {
    std::atomic_store(&m_counter, std::atomic_load(&other.m_counter));
    std::atomic_store(&other.m_counter, std::shared_ptr<std::atomic<int64_t>>(nullptr));
  }
  Barrier& operator=(const Barrier& copy) {
    std::atomic_load(&m_counter)->fetch_sub(1);
    std::atomic_store(&m_counter, std::atomic_load(&copy.m_counter));
    std::atomic_load(&m_counter)->fetch_add(1);
    return *this;
  }
  Barrier& operator=(Barrier&& other) {
    if (m_counter)
      std::atomic_load(&m_counter)->fetch_sub(1);
    std::atomic_store(&m_counter, std::atomic_load(&other.m_counter));
    std::atomic_store(&other.m_counter, std::shared_ptr<std::atomic<int64_t>>(nullptr));
    return *this;
  }
  bool done() const {
    return std::atomic_load(&m_counter)->load() <= 0;
  }
  void kill() {
    if (m_counter) {
      auto local = std::atomic_load(&m_counter);
      if (local->load() > 0)
        local->store(-100);
    }
  }
  explicit operator bool() const {
    return bool(std::atomic_load(&m_counter));
  }
  ~Barrier(){
    auto local = std::atomic_load(&m_counter);
    if (local)
      local->fetch_sub(1);
    std::atomic_store(&m_counter, std::shared_ptr<std::atomic<int64_t>>(nullptr));
  }
};

class BarrierObserver 
{
  std::shared_ptr<std::atomic<int64_t>> m_counter;
  public:
  BarrierObserver()
  {
    std::atomic_store(&m_counter, std::make_shared<std::atomic<int64_t>>(0));
  }
  BarrierObserver(const Barrier& barrier)
  {
    std::atomic_store(&m_counter, std::atomic_load(&barrier.m_counter));
  }
  BarrierObserver(const BarrierObserver& copy)
  {
    std::atomic_store(&m_counter, std::atomic_load(&copy.m_counter));
  }
  BarrierObserver(BarrierObserver&& other)
  {
    std::atomic_store(&m_counter, std::atomic_load(&other.m_counter));
    std::atomic_store(&other.m_counter, std::shared_ptr<std::atomic<int64_t>>(nullptr));
  }
  BarrierObserver& operator=(const BarrierObserver& copy) {
    std::atomic_store(&m_counter, std::atomic_load(&copy.m_counter));
    return *this;
  }
  BarrierObserver& operator=(BarrierObserver&& other) {
    std::atomic_store(&m_counter, std::atomic_load(&other.m_counter));
    std::atomic_store(&other.m_counter, std::shared_ptr<std::atomic<int64_t>>(nullptr));
    return *this;
  }
  Barrier barrier() {
    Barrier bar;
    auto local = std::atomic_load(&m_counter);
    local->fetch_add(1);
    std::atomic_store(&bar.m_counter, local);
    return bar;
  }
  bool done() const {
    if (!m_counter)
      return true;
    return std::atomic_load(&m_counter)->load() <= 0;
  }
  explicit operator bool() const {
    return bool(std::atomic_load(&m_counter));
  }
  ~BarrierObserver(){
  } 
};

class Task
{
public:
  Task() :
    m_id(0),
    m_iterations(0),
    m_iterID(0),
    m_ppt(1),
    m_sharedWorkCounter(std::shared_ptr<std::atomic<size_t>>(new std::atomic<size_t>()))
  {
    //genWorkFunc<1>([](size_t) {});
  };
  Task(size_t id, size_t start, size_t iterations, Barrier barrier) :
    m_id(id),
    m_iterations(iterations),
    m_iterID(start),
    m_ppt(1),
    m_sharedWorkCounter(std::shared_ptr<std::atomic<size_t>>(new std::atomic<size_t>())),
    m_blocks(std::move(barrier))
  {
    //genWorkFunc<1>([](size_t) {});
    m_sharedWorkCounter->store(m_iterations);
  };

private:
  Task(size_t id, size_t start, size_t iterations, std::shared_ptr<std::atomic<size_t>> sharedWorkCount, Barrier barrier) :
    m_id(id),
    m_iterations(iterations),
    m_iterID(start),
    m_ppt(1),
    m_sharedWorkCounter(sharedWorkCount),
    m_blocks(std::move(barrier))
  {
    //genWorkFunc<1>([](size_t) {});
  };
public:

  size_t m_id;
  size_t m_iterations;
  size_t m_iterID;
  int m_ppt;
  std::shared_ptr<std::atomic<size_t>> m_sharedWorkCounter;
  Barrier m_blocks; // maybe there is always one?...?

  std::function<bool(size_t&, size_t&)> f_work;

  // Generates ppt sized for -loop lambda inside this work.
  template<size_t ppt, typename Func>
  void genWorkFunc(Func&& func)
  {
    m_ppt = ppt;
    f_work = [func](size_t& iterID, size_t& iterations) mutable -> bool
    {
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
  }

  inline bool doWork()
  {
    return f_work(m_iterID, m_iterations);
  }

  inline bool canSplit()
  {
    return (m_iterations > static_cast<size_t>(m_ppt));
  }

  inline Task split()
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
  ThreadData() :m_ID(0), m_task(Task()), m_localQueueSize(std::make_shared<std::atomic<int>>(0)), m_alive(std::make_shared<std::atomic<bool>>(true)) { }
  ThreadData(int id, int logicalThread) :m_ID(id), m_logicalThread(logicalThread), m_task(Task()), m_localQueueSize(std::make_shared<std::atomic<int>>(0)), m_alive(std::make_shared<std::atomic<bool>>(true)) {  }

  int m_ID = 0;
  int m_logicalThread = 0;
  int m_core = 0;
  int m_l3group = 0;
  Task m_task;
  std::shared_ptr<std::atomic<int>> m_localQueueSize;
  std::deque<Task> m_localDeque;
  std::shared_ptr<std::atomic<bool>> m_alive;
};

struct CpuCore
{
  std::vector<int> logicalCores;
};

struct L3CacheCpuGroup
{
  uint64_t mask;
  std::vector<CpuCore> cores;
};

struct Numa
{
  DWORD number = 0;
  WORD processor = 0;
  int cores = 0;
  int threads = 0;
  std::vector<L3CacheCpuGroup> coreGroups;
};

class SystemCpuInfo {
  public:
  std::vector<Numa> numas;
  SystemCpuInfo() {
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION info = nullptr;
    DWORD infoLen = 0;
    if (!GetLogicalProcessorInformation(info, &infoLen)){
      auto error = GetLastError();
      if (error == ERROR_INSUFFICIENT_BUFFER) {
        info = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION>(malloc(infoLen));
        if (GetLogicalProcessorInformation(info, &infoLen)) {
          DWORD byteOffset = 0;
          PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = info;
          std::vector<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION> ptrs;
          while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= infoLen) {
            ptrs.push_back(ptr);
            switch(ptr->Relationship) {
              case RelationNumaNode:
              {
                GROUP_AFFINITY affi;
                bool woot = GetNumaNodeProcessorMaskEx(ptr->NumaNode.NodeNumber, &affi);
                numas.push_back({ptr->NumaNode.NodeNumber, affi.Group});
                break;
              }
              default:
                break;
            }
            byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
            ptr++;
          }

          for (auto& numa : numas) {
            for (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION pt : ptrs) {
              switch(pt->Relationship) {
                case RelationProcessorCore:
                {
                  break;
                }
                case RelationCache:
                {
                  if (pt->Cache.Level == 3) {
                    L3CacheCpuGroup group{};
                    group.mask = static_cast<uint64_t>(pt->ProcessorMask);
                    numa.coreGroups.push_back(group);
                  }
                  break;
                }
                case RelationProcessorPackage:
                {
                  break;
                }
                default:
                  break;
              }
            }
            for (auto& l3ca : numa.coreGroups)
              for (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION pt : ptrs) {
                auto res = l3ca.mask & pt->ProcessorMask;
                if (res != pt->ProcessorMask)
                  continue;
                switch(pt->Relationship) {
                  case RelationProcessorCore:
                  {
                    CpuCore core{};
                    auto mask = pt->ProcessorMask;
                    numa.cores++;
                    while(mask != 0) {
                      unsigned long index = -1;
                      if (_BitScanForward(&index, mask)!=0) {
                        core.logicalCores.push_back(static_cast<int>(index));
                        numa.threads++;
                        unsigned long unsetMask = 1 << index;
                        mask ^= unsetMask;
                      }
                      else{
                        break;
                      }
                    }
                    l3ca.cores.push_back(core);
                    break;
                  }
                  default:
                    break;
                }
              }
          }
        }
        free(info);
      }
    }
  }

};

#define LBSPOOL_ENABLE_PROFILE_THREADS
// hosts the worker threads for lbs_awaitable and works as the backend
thread_local bool thread_from_pool = false;
thread_local int thread_id = -1;
class LBSPool
{
  struct AllThreadData 
  {
    AllThreadData(int i, int logicalThread):data(i, logicalThread) {}
    ThreadData data;
    std::mutex mutex;
    std::condition_variable cv;
    std::vector<Task> addableTasks;
  };
  std::vector<std::thread> m_threads;
  std::vector<std::shared_ptr<AllThreadData>>  m_threadData;
  std::atomic<size_t>      m_nextTaskID; // just increasing index...
  std::atomic<bool> StopCondition;
  std::atomic<int> idle_threads;
  std::atomic<int> threads_awake;
  std::atomic<int> tasks_to_do;
  std::atomic<int> tasks_in_queue;
  std::atomic<size_t> tasks_done;

  std::mutex m_globalWorkMutex;
  std::vector<std::pair<std::vector<BarrierObserver>, Task>> m_taskQueue;

  void notifyAll(int whoNotified, bool ignoreTasks = false)
  {
    //HIGAN_CPU_FUNCTION_SCOPE();
    auto tasks = std::max(1, tasks_to_do.load());
    auto offset = std::max(0, whoNotified);
    const int tdSize = static_cast<int>(m_threadData.size());
    for (int i = 1; i < tdSize; ++i) 
    {
      auto& it = m_threadData[(i+offset)%tdSize];
      if ((tasks > 0 && it->data.m_ID != 0) || (ignoreTasks && it->data.m_ID != 0))
      {
        tasks--;
        it->cv.notify_one();
      }
    }
  }

  public:
  int tasksAdded() {return tasks_in_queue;}
  int tasksReadyToProcess() {return tasks_to_do;}
  int my_queue_count() {
    if (thread_from_pool)
      return m_threadData[thread_id]->data.m_localQueueSize->load(std::memory_order::relaxed);
    return 0;
  }
  int my_thread_index() {
    if (thread_from_pool)
      return thread_id;
    return 0;
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
    int procs = 0;//std::thread::hardware_concurrency();
    // control the amount of threads made.
    //procs = 4;
   
    SystemCpuInfo info;
    if (!info.numas.empty()) { // use the numa info
      auto& numa = info.numas.front();
      printf("found beautiful numa info!\n");
      printf("numas %zu\n", info.numas.size());
      printf("l3groups %zu\n", numa.coreGroups.size());
      printf("cores %d\n", numa.cores);
      printf("threads %d\n", numa.threads);
      procs = 0;
      bool hyperThreading = true;
      bool allCores = true;
      bool allGroups = true;
      bool splitCores = false; // create cores like 12341234 instead of current 11223344
      bool splitGroups = false; // create cores like 11332244 instead of 11223344, mix with above to get 13241324
      int group = 0;
      int logicalThreadsPerCore = (splitCores ? 1 : numa.threads / numa.cores);
      int coresPerGroup = (splitGroups ? 2 : numa.cores/static_cast<int>(numa.coreGroups.size()));
      for (int i = 0; i < numa.threads / numa.cores; i += logicalThreadsPerCore) {
        for (int k = 0; k < numa.coreGroups.size(); k+= coresPerGroup){
          for (auto& ccx : numa.coreGroups) {
            for (int ci = 0; ci < coresPerGroup; ci++) {
              auto& core = ccx.cores[k+ci];
              for (int lt = 0; lt < logicalThreadsPerCore; lt++) {
                auto& thread = core.logicalCores[lt+i];
                m_threadData.emplace_back(std::make_shared<AllThreadData>(procs, thread));
                auto& data = m_threadData.back()->data;
                data.m_l3group = group;
                data.m_core = core.logicalCores.front() / static_cast<int>(core.logicalCores.size());
                procs++;
                if (!hyperThreading)
                  break;
              }
              if (!allCores)
                break;
            }
            if (!allGroups)
              break;
            group++;
          }
        }
      }
    }
    else // fallback to old
    {
      printf("no numas here rip, fallback to old\n");
      procs = std::thread::hardware_concurrency();
      for (int i = 0; i < procs; i++)
      {
        m_threadData.emplace_back(std::make_shared<AllThreadData>(i, i));
      }
    }
    fflush(stdout);

    BOOL res = SetThreadAffinityMask(GetCurrentThread(), 1u<<m_threadData[0]->data.m_logicalThread);
    assert(res);
    for (auto&& it : m_threadData)
    {
      if (it->data.m_ID == 0) // Basically mainthread is one of *us*
        continue; 
      m_threads.push_back(std::thread(&LBSPool::loop, this, it->data.m_ID));
      res = SetThreadAffinityMask(m_threads.back().native_handle(), 1u << it->data.m_logicalThread);
      assert(res);
    }
    HIGAN_CPU_BRACKET("waiting for threads to wakeup for instant business.");
    while(threads_awake != procs-1);
  }
  void resetIDs() { m_nextTaskID = 1;}
  LBSPool(const LBSPool& asd) = delete;
  LBSPool(LBSPool&& asd) = delete;
  LBSPool& operator=(const LBSPool& asd) = delete;
  LBSPool& operator=(LBSPool&& asd) = delete;
  ~LBSPool() // you do not simply delete this
  {
    HIGAN_CPU_FUNCTION_SCOPE();
    StopCondition.store(true);
    for (auto&& data : m_threadData) {
      if (data->data.m_ID !=0)
        while (data->data.m_alive->load())
          notifyAll(-1, true);
    }
    for (auto& it : m_threads)
    {
      it.join();
    }
  }
  void loop(int i)
  {
    thread_from_pool = true;
    thread_id = i;
    threads_awake++;
    std::string threadName = "Thread ";
    threadName += std::to_string(i);
    threadName += '\0';
    AllThreadData& p = *m_threadData.at(i);
    {
#if defined(LBSPOOL_ENABLE_PROFILE_THREADS)
      HIGAN_CPU_BRACKET("thread - First steal!");
#endif
      stealOrWait(p);
    }
    while (!StopCondition)
    {
#if defined(LBSPOOL_ENABLE_PROFILE_THREADS)
      HIGAN_CPU_BRACKET(threadName.c_str());
#endif
      // can we split work?
      {
        if (p.data.m_task.canSplit() && tasks_to_do < 128)
        {
#if defined(LBSPOOL_ENABLE_PROFILE_THREADS)
          HIGAN_CPU_BRACKET("try split task");
#endif
          if (p.data.m_localQueueSize->load() == 0)
          { // Queue didn't have anything, adding.
            {
              std::lock_guard<std::mutex> guard(p.mutex);
              p.data.m_localDeque.push_back(p.data.m_task.split()); // push back
              p.data.m_localQueueSize->store(p.data.m_localDeque.size(), std::memory_order_relaxed);
              tasks_to_do++;
            }
            notifyAll(p.data.m_ID);
            continue;
          }
        }
      }
      // we couldn't split or queue had something
      if (doWork(p))
      {
        // out of work, steal work? sleep?
        stealOrWait(p);
      }
    }
    p.data.m_alive->store(false);
    threads_awake--;
    thread_from_pool = false;
    thread_id = 0;
  }

  inline bool stealOrSleep(AllThreadData& data, bool allowedToSleep = true) {
    //HIGAN_CPU_FUNCTION_SCOPE();
    // check my own deque
    auto& p = data.data;
    if (p.m_localQueueSize->load() > 0)
    {
      // try to take work from own deque, backside.
      std::lock_guard<std::mutex> guard(data.mutex);
      if (!p.m_localDeque.empty())
      {
        p.m_task = std::move(p.m_localDeque.back());
        p.m_localDeque.pop_back();
        p.m_localQueueSize->store(p.m_localDeque.size(), std::memory_order_relaxed);
        tasks_to_do--;
        return true;
      }
    }
    // other deques
    
    //for (auto &it : m_threadData)
    const int tdSize = static_cast<int>(m_threadData.size());
    for (int i = 1; i < tdSize; ++i) 
    {
      auto& it = m_threadData[(i+p.m_ID)%tdSize];
      if (it->data.m_l3group != p.m_l3group)
        continue;
      if (it->data.m_localQueueSize->load(std::memory_order::seq_cst) > 0) // this should reduce unnecessary lock_guards, and cheap.
      {
        std::unique_lock<std::mutex> guard(it->mutex, std::defer_lock_t());
        if (guard.try_lock() && !it->data.m_localDeque.empty()) // double check as it might be empty now.
        {
          p.m_task = it->data.m_localDeque.front();
          assert(p.m_task.m_iterations != 0);
          it->data.m_localDeque.pop_front();
          it->data.m_localQueueSize->store(it->data.m_localDeque.size());
          tasks_to_do--;
          return true;
        }
      }
    }
#if 1
    for (int i = 0; i < tdSize-1; ++i) 
    {
      auto& it = m_threadData[(i+p.m_ID)%tdSize];
      if (it->data.m_l3group == p.m_l3group)
        continue;
      if (it->data.m_localQueueSize->load(std::memory_order::relaxed) > 0) // this should reduce unnecessary lock_guards, and cheap.
      {
        std::unique_lock<std::mutex> guard(it->mutex, std::defer_lock_t());
        if (guard.try_lock() && !it->data.m_localDeque.empty()) // double check as it might be empty now.
        {
          p.m_task = it->data.m_localDeque.front();
          assert(p.m_task.m_iterations != 0);
          it->data.m_localDeque.pop_front();
          it->data.m_localQueueSize->store(it->data.m_localDeque.size());
          tasks_to_do--;
          return true;
        }
      }
    }
#endif
    // as last effort, check global queue
    if (tryTakeTaskFromGlobalQueue(data)) {
      return true;
    }

    // if all else fails, wait for more work.
    if (!StopCondition && allowedToSleep) // this probably doesn't fix the random deadlock
    {
      if (p.m_localQueueSize->load() != 0 || tasks_to_do > 0)
      {
        return false;
      }
      // potentially keep 1 thread always alive? for responsiveness? I don't know.
      /*
      if (idle_threads.fetch_add(1) >= threads_awake)
      {
        idle_threads.fetch_sub(1);
        //HIGAN_CPU_BRACKET("short sleeping");
        return false;
      }*/
#if defined(LBSPOOL_ENABLE_PROFILE_THREADS)
      HIGAN_CPU_BRACKET("try sleeping...");
#endif
      std::unique_lock<std::mutex> lk(data.mutex);
      idle_threads++;
      data.cv.wait(lk); // thread sleep
      idle_threads--;
    }
    return false;
  }

  inline void stealOrWait(AllThreadData& data, bool allowedToSleep = true)
  {
#if defined(LBSPOOL_ENABLE_PROFILE_THREADS)
    HIGAN_CPU_FUNCTION_SCOPE();
#endif
    ThreadData& p = data.data;
    p.m_task.m_id = 0;
    while (0 == p.m_task.m_id && !StopCondition)
    {
      stealOrSleep(data, allowedToSleep);
    }
  }
  bool tryTakeTaskFromGlobalQueue(AllThreadData& data)
  {
#if defined(LBSPOOL_ENABLE_PROFILE_THREADS)
    HIGAN_CPU_FUNCTION_SCOPE();
#endif
    std::vector<Task>& addable = data.addableTasks;
    if (tasks_in_queue.load(std::memory_order::relaxed) > 0)
    {
      if (!addable.empty())
        addable.clear();
#if defined(LBSPOOL_ENABLE_PROFILE_THREADS)
      HIGAN_CPU_BRACKET("getting task from global queue");
#endif
      std::unique_lock<std::mutex> guard(m_globalWorkMutex, std::defer_lock_t());
      if (guard.try_lock()) {
        m_taskQueue.erase(std::remove_if(m_taskQueue.begin(), m_taskQueue.end(), [&](const std::pair<std::vector<BarrierObserver>, Task>& it){
          if (allDepsDone(it.first)) {
            addable.push_back(std::move(it.second));
            return true;
          }
          return false;
        }), m_taskQueue.end());
      }
      tasks_in_queue -= static_cast<int>(addable.size());
    }
    if (!addable.empty())
    {
#if defined(LBSPOOL_ENABLE_PROFILE_THREADS)
      HIGAN_CPU_BRACKET("adding tasks to own queue");
#endif
      std::lock_guard<std::mutex> guard(data.mutex);
      data.data.m_localDeque.insert(data.data.m_localDeque.end(), addable.begin(), addable.end());
      data.data.m_localQueueSize->store(data.data.m_localDeque.size(), std::memory_order_relaxed);
      tasks_to_do += static_cast<int>(addable.size());
      notifyAll(data.data.m_ID);
      addable.clear();
      return true;
    }
    return false;
  }

  void informTaskFinished(AllThreadData& data)
  {
#if defined(LBSPOOL_ENABLE_PROFILE_THREADS)
    HIGAN_CPU_FUNCTION_SCOPE();
#endif
    data.data.m_task.m_id = 0;
    BarrierObserver observs = data.data.m_task.m_blocks;
    data.data.m_task.m_blocks.kill();
    data.data.m_task.m_blocks = Barrier();
    if (observs.done()) // possibly something done
    {
      tryTakeTaskFromGlobalQueue(data);
    }
  }

  inline void didWorkFor(AllThreadData& data, size_t amount) // Task specific counter this time
  {
#if defined(LBSPOOL_ENABLE_PROFILE_THREADS)
    HIGAN_CPU_FUNCTION_SCOPE();
#endif
    if (amount <= 0)
    {
      // did nothing, so nothing to report? wtf
      assert(false);
      return;
    }
    if (data.data.m_task.m_sharedWorkCounter->fetch_sub(amount) - amount <= 0) // be careful with the fetch_sub command
    {
      informTaskFinished(data);
    }
    if (data.data.m_task.m_id == 0 || data.data.m_task.m_iterations == 0){
      data.data.m_task = Task();
      tasks_done++;
    }
  }

  inline bool doWork(AllThreadData& data)
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

  inline bool allDepsDone(const std::vector<BarrierObserver>& barrs) {
    for (const auto& barrier : barrs) {
      if (!barrier.done())
        return false;
    }
    return true;
  }

  bool wantToAddTask(std::vector<BarrierObserver>&& depends) {
    int queue_count =  my_queue_count();
    bool depsDone = allDepsDone(depends);
    bool spawnTask = !depends.empty() || !depsDone || queue_count <= 10;
    //tasksReadyToProcess() < threads_awake.load(std::memory_order::relaxed)
    spawnTask = !((depends.empty() || depsDone) && queue_count > 2);
    return spawnTask;
  }

  template<typename Func>
  inline BarrierObserver task(std::vector<BarrierObserver>&& depends, Func&& func) {
    return internalAddTask<1>(std::forward<decltype(depends)>(depends), 0, 1, std::forward<Func>(func));
  }

  template<size_t ppt, typename Func>
  inline BarrierObserver internalAddTask(std::vector<BarrierObserver>&& depends, size_t start_iter, size_t iterations, Func&& func)
  {
#if defined(LBSPOOL_ENABLE_PROFILE_THREADS)
    HIGAN_CPU_FUNCTION_SCOPE();
#endif
    // need a temporary storage for tasks that haven't had requirements filled
    int ThreadID = my_thread_index();
    size_t newId = m_nextTaskID.fetch_add(1);
    assert(newId < m_nextTaskID);
    //printf("task added %zd\n", newId);
    BarrierObserver obs;
    Task newTask(newId, start_iter, iterations, std::move(obs.barrier()));
    auto& threadData = m_threadData.at(ThreadID);
    newTask.genWorkFunc<ppt>(std::forward<Func>(func));
    {
      if (depends.empty() || allDepsDone(depends))
      {
#if defined(LBSPOOL_ENABLE_PROFILE_THREADS)
        HIGAN_CPU_BRACKET("own queue");
#endif
        std::unique_lock<std::mutex> u2(threadData->mutex);
        threadData->data.m_localDeque.push_front(std::move(newTask));
        threadData->data.m_localQueueSize->store(threadData->data.m_localDeque.size(), std::memory_order_relaxed);
        tasks_to_do++;
      }
      else  // wtf add to WAITING FOR requi
      {
#if defined(LBSPOOL_ENABLE_PROFILE_THREADS)
        HIGAN_CPU_BRACKET("global queue");
#endif
        std::unique_lock<std::mutex> u1(m_globalWorkMutex);
        m_taskQueue.push_back({ std::move(depends), std::move(newTask) });
        tasks_in_queue++;
      }
    }
    notifyAll(-1);
    //while(!obs.done()){}
    //std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return obs;
  }

  void helpTasksUntilBarrierComplete(BarrierObserver observed) {
    assert(my_thread_index() == 0);
#if defined(LBSPOOL_ENABLE_PROFILE_THREADS)
    HIGAN_CPU_FUNCTION_SCOPE();
#endif
    auto& p = *m_threadData.at(0);
    while (!observed.done())
    {
      if (p.data.m_task.m_id == 0)
        stealOrSleep(p, false);
      if (p.data.m_task.m_id != 0) {
        // can we split work?
        if (p.data.m_task.canSplit())
        {
          if (p.data.m_localQueueSize->load() == 0)
          { // Queue didn't have anything, adding.
            {
              std::lock_guard<std::mutex> guard(p.mutex);
              p.data.m_localDeque.push_back(p.data.m_task.split()); // push back
              p.data.m_localQueueSize->store(p.data.m_localDeque.size(), std::memory_order_relaxed);
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

  size_t tasksDone() const { return tasks_done;}
};
struct noop_task {
  struct promise_type {
    noop_task get_return_object() noexcept {
      return { std::experimental::coroutine_handle<promise_type>::from_promise(*this) };
    }
    void unhandled_exception() noexcept {}
    void return_void() noexcept {}
    suspend_always initial_suspend() noexcept { return {}; }
    suspend_never final_suspend() noexcept { return {}; }
  };
  std::experimental::coroutine_handle<> coro;
};

std::experimental::coroutine_handle<> noop_coroutine() {
  return []() -> noop_task {
    co_return;
  }().coro;
}
std::experimental::coroutine_handle<> noop_coroutine(Barrier dep) {
  return [b = std::move(dep)]() mutable -> noop_task {
    b.kill();
    co_return;
  }().coro;
}

static std::shared_ptr<LBSPool> my_pool = nullptr;
thread_local void* thread_check_value = nullptr;
thread_local int arpa = 1;

template<typename T>
class lbs_awaitable {
public:
  struct promise_type {
    using coro_handle = std::experimental::coroutine_handle<promise_type>;
    __declspec(noinline) auto get_return_object() {
      return lbs_awaitable(coro_handle::from_promise(*this));
    }
    suspend_always initial_suspend() {
      return {};
    }

    struct final_awaiter {
      bool await_ready() noexcept {
        return false;
      }
      std::experimental::coroutine_handle<> await_suspend(coro_handle h) noexcept {
        // The coroutine is now suspended at the final-suspend point.
        // Lookup its continuation in the promise and resume it.
        /*
        auto handle = h.promise().continuation;
        auto& barrier = handle.promise().dependency;
        barrier = my_pool->task({h.promise().dependency}, [handlePtr = handle.address()](size_t) mutable {
          auto handle = coro_handle::from_address(handlePtr); 
          if (!handle.done()) {
            handle.resume();
          }
        });*/
        auto finalDep = h.promise().finalDependency;
        assert(!finalDep.done());
        //finalDependency = h.promise().finalDependency;
        void* ptr = h.address();

        if (!h.promise().async && h.promise().m_continuation.address() != nullptr){
          assert(thread_check_value != ptr);
          finalDep.kill();
          return h.promise().m_continuation;
        }
        //std::this_thread::sleep_for(std::chrono::milliseconds(100));
        //assert(!h.promise().finalDependency.done());
        //assert(h.promise().async && thread_check_value == ptr);
        //assert(h.promise().finalDependency.done());
        return noop_coroutine(std::move(h.promise().finalDependency));
      }
      void await_resume() noexcept {}
    };

    final_awaiter final_suspend() noexcept {
      return {};
    }
    void return_value(T value) {m_value = std::move(value);}
    void unhandled_exception() {
      std::terminate();
    }
    T m_value;
    bool async = false;
    bool isKilled = false;
    BarrierObserver bar;
    Barrier finalDependency;
    std::experimental::coroutine_handle<> m_continuation;
  };
  using coro_handle = std::experimental::coroutine_handle<promise_type>;
  lbs_awaitable(coro_handle handle) : handle_(handle)
  {
    assert(handle);
    handle_.promise().finalDependency = Barrier();
    if (true || my_pool->wantToAddTask({})) {
      handle_.promise().async = true;
      handle_.promise().bar = my_pool->task({}, [handlePtr = handle_.address()](size_t) mutable {
        thread_check_value = handlePtr;
        auto handle = coro_handle::from_address(handlePtr); 
        handle.resume();
        assert(thread_check_value == handlePtr);
        thread_check_value = nullptr;
        //if (handle.done()) handle.promise().finalDependency.kill();
      });
    }
  }
  lbs_awaitable(lbs_awaitable& other) {
    handle_ = other.handle_;
  };
  lbs_awaitable(lbs_awaitable&& other) {
    if (other.handle_)
      handle_ = std::move(other.handle_);
    assert(handle_);
    other.handle_ = nullptr;
  }
  lbs_awaitable& operator=(lbs_awaitable& other) {
    handle_ = other.handle_;
    return *this;
  };
  lbs_awaitable& operator=(lbs_awaitable&& other) {
    if (other.handle_)
      handle_ = std::move(other.handle_);
    assert(handle_);
    other.handle_ = nullptr;
    return *this;
  }

  // coroutine meat
  T await_resume() noexcept {
    return handle_.promise().m_value;
  }
  bool await_ready() noexcept {
    return handle_.done();
  }

  // enemy coroutine needs this coroutines result, therefore we compute it.
  std::experimental::coroutine_handle<> await_suspend(coro_handle handle) noexcept {
    /*
    auto& barrier = observer();
    auto& enemyBarrier = handle.promise().dependency;
    std::shared_ptr<coro_handle> otherHandle = handle.promise().weakref.lock();
    enemyBarrier = my_pool->task({handle_->promise().wholeCoroReady,barrier, enemyBarrier}, [handlePtr = otherHandle](size_t) mutable {
      if (!handlePtr->done()) {
        handlePtr->resume();
        if (handlePtr->done())
        {
          handlePtr->promise().wholeCoroReady.kill();
        }
      }
    });
    handle.promise().dependency = enemyBarrier;
    */
    if(handle_.promise().async)
    {
      //if (handle.address() != nullptr && !handle.done()) {
        void* ptr = handle.address();
        //assert(thread_check_value == ptr);
        BarrierObserver obs;
        auto finalDep = handle_.promise().finalDependency;
        if (finalDep)
          obs = BarrierObserver(finalDep);
        if (obs.done() && handle_.promise().bar.done())
        {
          //assert(handle_.done());
        }
        else
        {
          //assert(!handle_.done());
        }
        bool wasIAsync = handle.promise().async;
        Barrier temp;
        BarrierObserver tobs(temp);
        handle.promise().async = true;
        /*
        if (thread_check_value != handle.address())
          assert(thread_check_value == handle_.address());*/
        //assert(thread_check_value == handle.address());
        handle.promise().bar = my_pool->task({std::move(obs), handle_.promise().bar, handle.promise().bar, tobs}, [handlePtr = handle.address()](size_t) mutable {
          thread_check_value = handlePtr;
          auto handle = coro_handle::from_address(handlePtr);
          assert(!handle.done());
          handle.resume();
          assert(thread_check_value == handlePtr);
          thread_check_value = nullptr;
          //if (handle.done())
          //  handle.promise().finalDependency.kill();
        });
        //if (wasIAsync)
        //  std::this_thread::sleep_for(std::chrono::milliseconds(10));
      //}
      //else {
      //  assert(false);
      //}
      return noop_coroutine(temp);
    }
    handle_.promise().m_continuation = handle;
    return handle_;
  }
  // :o
  ~lbs_awaitable() {
    if (handle_ && handle_.done()) {
      assert(handle_.done());
      assert(!handle_.promise().isKilled);
      handle_.promise().isKilled = true;
      handle_.destroy();
    }
  }

  __declspec(noinline) T get()
  {
    auto obs = BarrierObserver(handle_.promise().finalDependency);
    bool wasIAsync = handle_.promise().async;
    while(!handle_.done()){
      if (handle_.promise().async)
        my_pool->helpTasksUntilBarrierComplete(obs);
      else {
        handle_.resume();
        if (!wasIAsync && handle_.promise().async)
        {
          wasIAsync = true;
          printf("Am async now! \n");
        }
      }
    }
    // we are exiting here too fast, somebody with stack left and our destruction kills it.
    assert(BarrierObserver(handle_.promise().finalDependency).done());
    //assert(handle_.promise().bar.done());
    assert(handle_.done());
    return handle_.promise().m_value; 
  }
  // unwrap() future<future<int>> -> future<int>
  // future then(lambda) -> attach function to be done after current task.
  // is_ready() are you ready?
private:
  std::experimental::coroutine_handle<promise_type> handle_;
};

namespace {
    std::uint64_t Fibonacci(std::uint64_t number) {
        return number < 2 ? 1 : Fibonacci(number - 1) + Fibonacci(number - 2);
    }

    async_awaitable<uint64_t> FibonacciOrig(uint64_t number) {
        co_return Fibonacci(number);
    }
    lbs_awaitable<uint64_t> FibonacciCoro(uint64_t number, uint64_t parallel) {
        if (number < 2)
            co_return 1;
        
        if (number > parallel) {
            auto v0 = FibonacciCoro(number-1, parallel);
            auto v1 = FibonacciCoro(number-2, parallel);
            co_return co_await v0 + co_await v1;
        }
        auto fib0 = Fibonacci(number - 1);
        auto fib1 = Fibonacci(number - 2);
        co_return fib0 + fib1;
    }

    async_awaitable<uint64_t> FibonacciAsync(uint64_t number, uint64_t parallel) {
        if (number < 2)
            co_return 1;
        
        if (number > parallel) {
            auto v0 = FibonacciAsync(number-1, parallel);
            auto v1 = FibonacciAsync(number-2, parallel);
            co_return co_await v0 + co_await v1;
        }
        co_return Fibonacci(number - 1) + Fibonacci(number - 2);
    }
}


TEST_CASE("Benchmark Fibonacci", "[benchmark]") {

    my_pool = std::make_shared<LBSPool>();
    
    CHECK(FibonacciOrig(0).get() == 1);
    // some more asserts..
    CHECK(FibonacciOrig(5).get() == 8);
    // some more asserts..
    CHECK(FibonacciCoro(0, 0).get() == 1);
    // some more asserts..
    CHECK(FibonacciCoro(5, 5).get() == 8);
    // some more asserts..
    uint64_t parallel = 6;
    BENCHMARK("Fibonacci 25") {
        return FibonacciOrig(25).get();
    };
    BENCHMARK("Coroutine Fibonacci 25") {
        return FibonacciCoro(25, 25-parallel).get();
    };
    BENCHMARK("Fibonacci 30") {
        return FibonacciOrig(30).get();
    };
    BENCHMARK("Coroutine Fibonacci 30") {
        return FibonacciCoro(30,30-parallel).get();
    };
    
    BENCHMARK("Fibonacci 32") {
        return FibonacciOrig(32).get();
    };
    BENCHMARK("Coroutine Fibonacci 32") {
        return FibonacciCoro(32,32-parallel).get();
    };
    
    BENCHMARK("Fibonacci 36") {
        return FibonacciOrig(36).get();
    };
    BENCHMARK("Coroutine Fibonacci 36") {
        return FibonacciCoro(36,36-parallel-1).get();
    };

    BENCHMARK("Fibonacci 38") {
        return FibonacciOrig(38).get();
    };
    BENCHMARK("Coroutine Fibonacci 38") {
        return FibonacciCoro(38,38-parallel-2).get();
    };
    

  //higanbana::FileSystem fs("/../../tests/data/");
  //higanbana::profiling::writeProfilingData(fs);
}
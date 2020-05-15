#define CATCH_CONFIG_DISABLE_EXCEPTIONS
#include <catch2/catch.hpp>

#include <higanbana/core/system/time.hpp>
#include <higanbana/core/profiling/profiling.hpp>

#include <vector>
#include <thread>
#include <future>
#include <optional>
#include <cstdio>
#include <iostream>
#include <deque>
#include <experimental/coroutine>
#include <atomic>

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

class resumable {
public:
  struct promise_type {
    using coro_handle = std::experimental::coroutine_handle<promise_type>;
    auto get_return_object() {
      return coro_handle::from_promise(*this);
    }
    auto initial_suspend() { return suspend_always(); }
    auto final_suspend() noexcept { return suspend_always(); }
    void return_void() {}
    void unhandled_exception() {
      std::terminate();
    }
  };
  using coro_handle = std::experimental::coroutine_handle<promise_type>;
  resumable(coro_handle handle) : handle_(handle)
  {
    static int woot = 0;
    ver = woot++;
    printf("resumable created %d \n", ver);
    assert(handle);
  }
  resumable(resumable& other) = delete;
  resumable(resumable&& other) : handle_(std::move(other.handle_)) {
    assert(handle_);
    other.handle_ = nullptr;
  }
  bool resume() noexcept {
    if (res > 0) {
      res = res - 1;
      return !handle_.done();
    }
    if (!handle_.done())
      handle_.resume();
    return !handle_.done();
  }
  // coroutine meat
  void await_resume() noexcept {
    printf("resumable await_resume%d \n", ver);
    resume();
  }
  bool await_ready() noexcept {
    printf("resumable await_ready%d \n", ver);
    return handle_.done();
  }

  // enemy coroutine needs this coroutines result, therefore we compute it.
  void await_suspend(coro_handle handle) noexcept {
    printf("resumable await_suspend%d \n", ver);
    while(resume());
  }
  // :o
  ~resumable() { if (handle_) handle_.destroy(); }
private:
  coro_handle handle_;
  int ver;
  int res = 2;
};
resumable hehe_suspendi(int i){
  std::cout << "suspendddd " << i << std::endl;
  co_await suspend_always();
  std::cout << "finishhhhh " << i << std::endl;
}

resumable hehe_suspend2(){
  std::cout << "suspendddd2" << std::endl;
  co_await hehe_suspendi(3);
  std::cout << "finishhhhh2" << std::endl;
}

resumable hehe_suspend(){
  std::cout << "suspendddd" << std::endl;
  co_await hehe_suspend2();
  std::cout << "finishhhhh" << std::endl;
}

resumable foo(){
  std::cout << "Hello" << std::endl;
  co_await hehe_suspend();
  std::cout << "middle hello" << std::endl;
  co_await hehe_suspend();
  std::cout << "Coroutine" << std::endl;
}

/*
TEST_CASE("c++ threading")
{
  resumable res = foo();
  std::cout << "created resumable" << std::endl;
  while (res.resume()) {std::cout << "resume coroutines!!" << std::endl;}
}*/

int addInTreeNormal(int treeDepth) {
  if (treeDepth <= 0)
    return 1;
  int sum = 0;
  sum += addInTreeNormal(treeDepth-1);
  sum += addInTreeNormal(treeDepth-1);
  return sum;
}

std::future<int> addInTreeAsync(int treeDepth) {
  if (treeDepth <= 0)
    co_return 1;
  int result=0;
  result += co_await addInTreeAsync(treeDepth-1);
  result += co_await addInTreeAsync(treeDepth-1);
  co_return result;
}

/*
TEST_CASE("simple recursive async")
{
  int treeSize = 20;
  higanbana::Timer time;
  int basecase = addInTreeNormal(treeSize);
  auto normalTime = time.reset();
  int futured = addInTreeAsync(treeSize).get();
  auto asyncTime = time.reset();
  REQUIRE(basecase == futured);
  printf("normal %.3fms vs coroutined %.3fms\n", normalTime/1000.f/1000.f, asyncTime/1000.f/1000.f);
}
*/

template<typename T>
class awaitable {
public:
  struct promise_type {
    using coro_handle = std::experimental::coroutine_handle<promise_type>;
    auto get_return_object() {
      return coro_handle::from_promise(*this);
    }
    auto initial_suspend() { return suspend_always(); }
    auto final_suspend() noexcept { return suspend_always(); }
    void return_value(T value) {m_value = value;}
    void unhandled_exception() {
      std::terminate();
    }
    auto await_transform(awaitable<T> handle) {
      return handle;
    }
    T m_value;
  };
  using coro_handle = std::experimental::coroutine_handle<promise_type>;
  awaitable(coro_handle handle) : handle_(handle)
  {
    assert(handle);
  }
  awaitable(awaitable& other) = delete;
  awaitable(awaitable&& other) : handle_(std::move(other.handle_)) {
    assert(handle_);
    other.handle_ = nullptr;
  }
  bool resume() noexcept {
    if (!handle_.done())
      handle_.resume();
    return !handle_.done();
  }
  // coroutine meat
  T await_resume() noexcept {
    while(resume());
    return handle_.promise().m_value;
  }
  bool await_ready() noexcept {
    return handle_.done();
  }

  // enemy coroutine needs this coroutines result, therefore we compute it.
  void await_suspend(coro_handle handle) noexcept {
    while(resume());
  }
  // :o
  ~awaitable() { if (handle_) handle_.destroy(); }
  T get()
  {
    while(resume());
    assert(handle_.done());
    return handle_.promise().m_value; 
  }
private:
  coro_handle handle_;
};

template<>
class awaitable<void> {
public:
  struct promise_type {
    using coro_handle = std::experimental::coroutine_handle<promise_type>;
    auto get_return_object() {
      return coro_handle::from_promise(*this);
    }
    auto initial_suspend() { return suspend_always(); }
    auto final_suspend() noexcept { return suspend_always(); }
    void return_void() {}
    void unhandled_exception() {
      std::terminate();
    }
    auto await_transform(awaitable<void> handle) {
      return handle;
    }
  };
  using coro_handle = std::experimental::coroutine_handle<promise_type>;
  awaitable(coro_handle handle) : handle_(handle)
  {
    assert(handle);
  }
  awaitable(awaitable& other) = delete;
  awaitable(awaitable&& other) : handle_(std::move(other.handle_)) {
    assert(handle_);
    other.handle_ = nullptr;
  }
  bool resume() noexcept {
    if (!handle_.done())
      handle_.resume();
    return !handle_.done();
  }
  // coroutine meat
  void await_resume() noexcept {
    while(resume());
  }
  bool await_ready() noexcept {
    return handle_.done();
  }
  // enemy coroutine needs this coroutines result, therefore we compute it.
  void await_suspend(coro_handle handle) noexcept {
    while(resume());
  }
  // :o
  ~awaitable() { if (handle_) handle_.destroy(); }
private:
  coro_handle handle_;
};

awaitable<int> funfun() {
  co_return 1;
}

awaitable<void> funfun2() {
  printf("woot\n");
  co_return;
}

awaitable<int> addInTreeAsync2(int treeDepth) {
  if (treeDepth <= 0)
    co_return 1;
  int result=0;
  result += co_await addInTreeAsync2(treeDepth-1);
  result += co_await addInTreeAsync2(treeDepth-1);
  co_return result;
}
/*
TEST_CASE("simple my future")
{
  auto fut = funfun().get();
  REQUIRE(fut == 1);
  funfun2().resume();
  printf("had value in my future %d\n", fut);
  int treeSize = 20;
  higanbana::Timer time;
  int basecase = addInTreeNormal(treeSize);
  time.reset();
  int futured2 = addInTreeAsync2(treeSize).get();
  auto asyncTime2 = time.reset();
  REQUIRE(basecase == futured2);
  printf("awaitable %.3fms\n", asyncTime2/1000.f/1000.f);
}*/

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
    std::weak_ptr<coro_handle> weakref;
  };
  using coro_handle = std::experimental::coroutine_handle<promise_type>;
  async_awaitable(coro_handle handle) : handle_(std::shared_ptr<coro_handle>(new coro_handle(handle), [](coro_handle* ptr){ptr->destroy(); delete ptr;}))
  {
    //printf("created async_awaitable\n");
    assert(handle);

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
    handle_->promise().finalFut.get_future().wait();
    //handle_->promise().m_fut.wait();
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
    auto finalFut = handle_->promise().finalFut.get_future();
    finalFut.wait();
    assert(handle_->done());
    return handle_->promise().m_value; 
  }
private:
  std::shared_ptr<coro_handle> handle_;
};

async_awaitable<int> funfun3() {
  co_return 1;
}

async_awaitable<int> funfun4() {
  int sum = 0;
  sum += co_await funfun3();
  co_return sum;
}

async_awaitable<int> addInTreeAsync3(int treeDepth, int parallelDepth) {
  if (treeDepth <= 0)
    co_return 1;
  if (treeDepth > parallelDepth) {
    int result = 0;
    auto res0 = addInTreeAsync3(treeDepth-1, parallelDepth);
    auto res1 = addInTreeAsync3(treeDepth-1, parallelDepth);
    result += co_await res1;
    result += co_await res0;
    co_return result;
  }
  else {
    auto res0 = addInTreeNormal(treeDepth-1);
    auto res1 = addInTreeNormal(treeDepth-1);
    co_return res0 + res1;
  }
}

/*
TEST_CASE("threaded awaitable")
{
  auto fut = funfun3().get();
  fut = funfun4().get();
  REQUIRE(fut == 1);
  int treeSize = 24;
  higanbana::Timer time;
  int basecase = addInTreeNormal(treeSize);
  auto normalTime = time.reset();
  int asynced = addInTreeAsync3(treeSize, treeSize-7).get();
  asynced = addInTreeAsync3(treeSize, treeSize-7).get();
  time.reset();
  asynced = addInTreeAsync3(treeSize, treeSize-7).get();
  auto asyncTime = time.reset();
  REQUIRE(basecase == asynced);
  printf("normal %.3fms awaitable %.3fms %.2f speedup\n", normalTime/1000.f/1000.f, asyncTime/1000.f/1000.f, float(normalTime)/float(asyncTime));
}
*/

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
    m_sharedWorkCounter(std::shared_ptr<std::atomic<size_t>>(new std::atomic<size_t>()))
  {
  };
  Task(size_t id, size_t start, size_t iterations, Barrier barrier) :
    m_id(id),
    m_iterations(iterations),
    m_iterID(start),
    m_ppt(1),
    m_sharedWorkCounter(std::shared_ptr<std::atomic<size_t>>(new std::atomic<size_t>())),
    m_blocks(std::move(barrier))
  {
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
  };
public:

  size_t m_id;
  size_t m_iterations;
  size_t m_iterID;
  std::shared_ptr<std::atomic<size_t>> m_sharedWorkCounter;
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
  int m_ID = 0;
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
    if (thread_from_pool)
      return m_threadData[thread_id].data.m_localQueueSize.load(std::memory_order::relaxed);
    return 0;
  }
  int my_thread_index() noexcept {
    if (thread_from_pool)
      return thread_id;
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
    thread_from_pool = true;
    thread_id = i;
    threads_awake++;
    std::string threadName = "Thread ";
    threadName += std::to_string(i);
    threadName += '\0';
    AllThreadData& p = m_threadData.at(i);
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
          if (p.data.m_localQueueSize.load(std::memory_order::relaxed) == 0)
          { // Queue didn't have anything, adding.
            {
              std::lock_guard<std::mutex> guard(*p.mutex);
              p.data.m_localDeque.emplace_back(std::move(p.data.m_task.split())); // push back
              p.data.m_localQueueSize.store(p.data.m_localDeque.size(), std::memory_order_relaxed);
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
    p.data.m_alive.store(false);
    threads_awake--;
    thread_from_pool = false;
    thread_id = 0;
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
    
    //for (auto &it : m_threadData)
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
    if (!StopCondition && allowedToSleep) // this probably doesn't fix the random deadlock
    {
      if (p.m_localQueueSize.load(std::memory_order::relaxed) != 0 || tasks_to_do.load(std::memory_order::relaxed) > 0)
      {
        return false;
      }
#if defined(LBSPOOL_ENABLE_PROFILE_THREADS)
      HIGAN_CPU_BRACKET("try sleeping...");
#endif
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

  bool tryTakeTaskFromGlobalQueue(AllThreadData& data) noexcept
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
    BarrierObserver observs = data.data.m_task.m_blocks;
    data.data.m_task.m_blocks.kill();
    data.data.m_task.m_blocks = Barrier();
    if (observs.done()) // possibly something done
    {
      tryTakeTaskFromGlobalQueue(data);
    }
  }

  inline void didWorkFor(AllThreadData& data, size_t amount) noexcept // Task specific counter this time
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

  bool wantToAddTask(std::vector<BarrierObserver>&& depends) noexcept {
    int queue_count =  my_queue_count();
    bool depsDone = allDepsDone(depends);
    bool spawnTask = !depends.empty() || !depsDone || queue_count <= 10;
    //tasksReadyToProcess() < threads_awake.load(std::memory_order::relaxed)
    spawnTask = !((depends.empty() || depsDone) && queue_count > 2);
    return spawnTask || true;
  }

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
        std::unique_lock<std::mutex> u2(*threadData.mutex);
        threadData.data.m_localDeque.push_back(std::move(newTask));
        threadData.data.m_localQueueSize.store(threadData.data.m_localDeque.size());
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

std::experimental::coroutine_handle<> noop_coroutine() noexcept{
  return []() -> noop_task {
    co_return;
  }().coro;
}
std::experimental::coroutine_handle<> noop_coroutine(Barrier dep) noexcept {
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
    __declspec(noinline) auto get_return_object() noexcept {
      return lbs_awaitable(coro_handle::from_promise(*this));
    }
    constexpr suspend_always initial_suspend() noexcept {
      return {};
    }

    struct final_awaiter {
      constexpr bool await_ready() noexcept {
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
    void return_value(T value) noexcept {m_value = std::move(value);}
    void unhandled_exception() noexcept {
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
  lbs_awaitable(coro_handle handle) noexcept : handle_(handle)
  {
    assert(handle);
    handle_.promise().finalDependency = Barrier();
    if (my_pool->wantToAddTask({})) {
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
  lbs_awaitable(lbs_awaitable& other) noexcept {
    handle_ = other.handle_;
  };
  lbs_awaitable(lbs_awaitable&& other) noexcept {
    if (other.handle_)
      handle_ = std::move(other.handle_);
    assert(handle_);
    other.handle_ = nullptr;
  }
  lbs_awaitable& operator=(lbs_awaitable& other) noexcept {
    handle_ = other.handle_;
    return *this;
  };
  lbs_awaitable& operator=(lbs_awaitable&& other) noexcept {
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
  ~lbs_awaitable() noexcept {
    if (handle_ && handle_.done()) {
      assert(handle_.done());
      assert(!handle_.promise().isKilled);
      handle_.promise().isKilled = true;
      handle_.destroy();
    }
  }

  T get() noexcept
  {
    auto obs = BarrierObserver(handle_.promise().finalDependency);
    bool wasIAsync = handle_.promise().async;
    while(!handle_.done()){
      if (handle_.promise().async)
        my_pool->helpTasksUntilBarrierComplete(obs);
      else {
        //assert(handle_.promise().async);
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

lbs_awaitable<int> funfun5() noexcept {
  co_return 1;
}

lbs_awaitable<int> funfun6() noexcept {
  int sum = 0;
  auto thing = funfun5();
  auto thing2 = funfun5();
  auto thing3 = funfun5();
  sum += co_await thing;
  sum += co_await thing2;
  sum += co_await thing3;
  co_return sum;
}

int funfun7() {
  return funfun6().get();
}

lbs_awaitable<int> addInTreeLBS(int treeDepth, int parallelDepth) noexcept {
  if (treeDepth <= 0)
    co_return 1;
  if (treeDepth > parallelDepth) {
    int result = 0;
    auto res0 = addInTreeLBS(treeDepth-1, parallelDepth);
    auto res1 = addInTreeLBS(treeDepth-1, parallelDepth);
    result += co_await res1;
    result += co_await res0;
    co_return result;
  }
  else {
    auto res0 = addInTreeNormal(treeDepth-1);
    auto res1 = addInTreeNormal(treeDepth-1);
    co_return res0 + res1;
  }
}
uint64_t Fibonacci(uint64_t number) noexcept {
  return number < 2 ? 1 : Fibonacci(number - 1) + Fibonacci(number - 2);
}
lbs_awaitable<uint64_t> FibonacciCoro(uint64_t number, uint64_t parallel) noexcept {
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


lbs_awaitable<int> asyncLoopTest(int treeSize, int computeTree, size_t compareTime) noexcept {
  higanbana::Timer time2;

  size_t mint = 0, maxt = 0;
  size_t avegMin = 0, avegMax = 0;
  size_t aveCount = 0;
  mint = compareTime;
  maxt = 0;
  auto omi = mint;
  auto oma = 0;
  auto overlap = addInTreeLBS(treeSize, treeSize-computeTree);
  size_t aveg = 0;
  for (int i = 0; i < 4000; i++) {
    //my_pool->resetIDs();
    //if (i % 100 == 0) {

    //}
    //funfun6().get();
    auto another = addInTreeLBS(treeSize, treeSize-computeTree);
    int lbs = co_await another;//overlap;
    //overlap = std::move(another);
    auto t = time2.reset();
    aveg += t;
    mint = (mint > t) ? t : mint;
    maxt = (maxt < t) ? t : maxt;
    if (i%100 == 0) {
      avegMin = avegMin + mint;
      avegMax = avegMax + maxt;
      aveCount++;
      printf("done %d ST:%.3fms %.2f ratio %.3fms %.3fms %.3fms\n",i, compareTime/1000.f/1000.f, float(compareTime) / float(aveg/100), aveg/100/1000.f/1000.f, mint/1000.f/1000.f, maxt/1000.f/1000.f);
      aveg = 0;
      mint = omi;
      maxt = oma;
      fflush(stdout);
    }
  }
  co_return co_await overlap;
}

TEST_CASE("threaded awaitable - lbs")
{
  {
    HIGAN_CPU_BRACKET("threaded awaitable - lbs");
    int computeTree = 6;
    int treeSize = 26;
    int a = addInTreeNormal(treeSize);
    a = addInTreeNormal(treeSize);
    a = addInTreeNormal(treeSize);
    a = addInTreeNormal(treeSize);
    auto time2 = higanbana::Timer();
    a = addInTreeNormal(treeSize);
    auto stTime = time2.reset();
    my_pool = std::make_shared<LBSPool>();
    auto fut = funfun5().get();
    fut = funfun6().get();
    
    REQUIRE(fut == 3);

    /*
    for (int i = 0; i < 10000; i++) {
      auto fibo = Fibonacci(20);
      auto fibo2 = fibo;
      try {
        fibo2 = FibonacciCoro(20, 7).get();
      } catch (...){
        assert(false);
      }
      REQUIRE(fibo == fibo2);
    }*/
    

    /*
    higanbana::Timer time2;
    auto overlap = addInTreeLBS(treeSize, treeSize-computeTree);
    for (int i = 0; i < 1000*1000; i++) {
      //my_pool->resetIDs();
      //if (i % 100 == 0) {

      //}
      //funfun6().get();
      auto another = addInTreeLBS(treeSize, treeSize-computeTree);
      int lbs = overlap.get();
      overlap = std::move(another);
      REQUIRE(basecase == lbs);
      auto t = time2.reset();
      printf("done %d %.3fms\n",i, t/1000.f/1000.f);
      fflush(stdout);
    }
    overlap.get();
    */
    higanbana::Timer time;
    int result = asyncLoopTest(treeSize, computeTree, stTime).get();
    printf("long loop done in %.3fms\n", time.reset()/1000.f/1000.f);
    int basecase2 = addInTreeNormal(treeSize) + a;
    REQUIRE(basecase2 == result);
    
    
    int basecase = addInTreeNormal(treeSize);
    auto normalTime = time.reset();
    int asynced = basecase;//addInTreeAsync3(treeSize, treeSize-computeTree).get();
    time.reset();
    //asynced = 0;// addInTreeAsync3(treeSize, treeSize-computeTree).get();
    REQUIRE(basecase == asynced);
    auto asyncTime = time.reset();
    //auto lbsed0 = addInTreeLBS(treeSize, treeSize-computeTree);
    auto lbsed = addInTreeLBS(treeSize, treeSize-computeTree).get();
    //lbsed0.get();
    REQUIRE(basecase == lbsed);
    auto lbsTime = time.reset();
    //printf("normal %.3fms awaitable %.3fms %.2f speedup\n", normalTime/1000.f/1000.f, asyncTime/1000.f/1000.f, float(normalTime)/float(asyncTime));
    printf("tasks done %zd normal %.3fms lbs %.3fms %.2f speedup\n", my_pool->tasksDone(), normalTime/1000.f/1000.f, lbsTime/1000.f/1000.f, float(normalTime)/float(lbsTime));
    
    
    /*
    LBSPool& pool = *my_pool;
    std::atomic_bool lel = true;
    higanbana::Timer time;
    size_t taskAddTime;
    BarrierObserver barrier4;
    {
      HIGAN_CPU_BRACKET("add tasks");
      auto barrier = pool.internalAddTask<1>(0, {}, 0, 1, [](size_t){
        HIGAN_CPU_BRACKET("first task");
      });
      auto barrier2 = pool.internalAddTask<1>(1, {barrier}, 0, 1, [](size_t){
        HIGAN_CPU_BRACKET("second task 0");
      });
      auto barrier3 = pool.internalAddTask<1>(2, {barrier}, 0, 1, [](size_t){
        HIGAN_CPU_BRACKET("second task 1");
      });
      barrier4 = pool.internalAddTask<1>(3, {barrier2, barrier3}, 0, 1, [&](size_t){
        HIGAN_CPU_BRACKET("final");
        lel = false;
      });
      taskAddTime = time.reset();
    }
    {
      HIGAN_CPU_BRACKET("wait tasks");
      //pool.helpTasksUntilBarrierComplete(barrier4);
      while(lel && !barrier4.done());
    }
    auto t = time.reset() + taskAddTime;
    printf("added tasks in %.4fms and total time till completion %.4fms\n", taskAddTime/1000.f/1000.f, t/1000.f/1000.f);

    std::atomic_int64_t addable;
    time.reset();
    BarrierObserver obs;
    for (int i = 0; i < 1024; ++i)
      pool.internalAddTask<1>(0, {}, 0, 1, [&](size_t){
      });
    obs = pool.internalAddTask<1>(0,{}, 0, 1, [&](size_t){
    });
    taskAddTime = time.reset();
    pool.helpTasksUntilBarrierComplete(obs);
    t = taskAddTime;
    printf("parallelfor - schedule overhead: total time %.4fms and total tasks produced and done %zu\n", t/1000.f/1000.f, pool.tasksDone());
    */
  }
  //higanbana::FileSystem fs("/../../tests/data/");
  //higanbana::profiling::writeProfilingData(fs);
}
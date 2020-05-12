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

class SuperLBS
{
  //std::vector<std::thread> m_threads;

};

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
  };
  using coro_handle = std::experimental::coroutine_handle<promise_type>;
  async_awaitable(coro_handle handle) : handle_(std::make_shared<coro_handle>(handle))
  {
    //printf("created async_awaitable\n");
    assert(handle);

    m_fut = std::async(std::launch::async, [handlePtr = handle_]
    {
      if (!handlePtr->done()) {
        //printf("trying to work\n");
        handlePtr->resume();
      }
    });
  }
  async_awaitable(async_awaitable& other) = delete;
  async_awaitable(async_awaitable&& other) : handle_(std::move(other.handle_)), m_fut(std::move(other.m_fut)) {
    //printf("moving\n");
    assert(handle_);
    other.handle_ = nullptr;
    //other.m_fut = nullptr;
  }

  bool launch_work_if_work_left() noexcept {
    if (m_fut.valid() && !m_fut._Is_ready())
      m_fut.wait();
    bool allWorkDone = handle_->done();
    if (!allWorkDone) {
      m_fut = std::async(std::launch::async, [handlePtr = handle_]
      {
      if (!handlePtr->done()) {
        //printf("trying to work some more\n");
        handlePtr->resume();
      }
      });
    }
    return !allWorkDone;
  }
  // coroutine meat
  T await_resume() noexcept {
    //printf("await_resume\n");
    while(launch_work_if_work_left());
    return handle_->promise().m_value;
  }
  bool await_ready() noexcept {
    //printf("await_ready\n");
    if (m_fut.valid())
      m_fut.wait();
    return handle_->done();
  }

  // enemy coroutine needs this coroutines result, therefore we compute it.
  void await_suspend(coro_handle handle) noexcept {
    //printf("await_suspend\n");
    while(launch_work_if_work_left());
  }
  // :o
  ~async_awaitable() { if (handle_) handle_->destroy(); }
  T get()
  {
    //printf("get\n");
    while(launch_work_if_work_left());
    assert(handle_->done());
    return handle_->promise().m_value; 
  }
private:
  std::shared_ptr<coro_handle> handle_;
  std::future<void> m_fut;
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
    result += res1.get();
    result += res0.get();
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

// can ask if all latches are released -> dependency is solved, can run the task
class Barrier
{
  friend class BarrierObserver;
  std::shared_ptr<std::atomic<int64_t>> m_counter;
  size_t taskid = 0;
  public:
  Barrier(int taskid = 0) : m_counter(std::make_shared<std::atomic<int64_t>>(1)), taskid(taskid)
  {
  }
  Barrier(const Barrier& copy){
    m_counter = copy.m_counter;
    taskid = copy.taskid;
    m_counter->fetch_add(1);
  }
  Barrier(Barrier&& other) : m_counter(std::move(other.m_counter)), taskid(other.taskid) {other.m_counter = nullptr;}
  Barrier& operator=(const Barrier& copy) {
    m_counter->fetch_sub(1);
    m_counter = copy.m_counter;
    taskid = copy.taskid;
    m_counter->fetch_add(1);
    return *this;
  }
  Barrier& operator=(Barrier&& other) {
    if (m_counter)
      *m_counter -= 1;
    m_counter = std::move(other.m_counter);
    taskid = other.taskid;
    other.m_counter = nullptr;
    return *this;
  }
  bool done() const {
    return m_counter->load() <= 0;
  }
  void kill() {
    *m_counter = -1;
  }
  ~Barrier(){
    if (m_counter)
      m_counter->fetch_sub(1);
  }
};

class BarrierObserver 
{
  std::shared_ptr<std::atomic<int64_t>> m_counter;
  size_t taskid = 0;
  public:
  BarrierObserver() : m_counter(std::make_shared<std::atomic<int64_t>>(0))
  {
  }
  BarrierObserver(Barrier barrier) : m_counter(barrier.m_counter), taskid(barrier.taskid)
  {
  }
  BarrierObserver(const BarrierObserver& copy) : m_counter(copy.m_counter), taskid(copy.taskid){
  }
  BarrierObserver(BarrierObserver&& other) : m_counter(std::move(other.m_counter)), taskid(other.taskid) {other.m_counter = nullptr;}
  BarrierObserver& operator=(const BarrierObserver& copy) {
    m_counter = copy.m_counter;
    taskid = copy.taskid;
    return *this;
  }
  BarrierObserver& operator=(BarrierObserver&& other) {
    m_counter = std::move(other.m_counter);
    taskid = other.taskid;
    other.m_counter = nullptr;
    return *this;
  }
  Barrier barrier() {
    Barrier bar;
    m_counter->fetch_add(1);
    bar.m_counter = m_counter;
    bar.taskid = taskid;
    return bar;
  }
  bool done() const {
    return m_counter->load() <= 0;
  }
  explicit operator bool() const {
    return bool(m_counter);
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
  ThreadData() :m_ID(0), m_task(Task()), m_localQueueSize(std::make_shared<std::atomic<int64_t>>(0)), m_alive(std::make_shared<std::atomic<bool>>(true)) { }
  ThreadData(int id) :m_ID(id), m_task(Task()), m_localQueueSize(std::make_shared<std::atomic<int64_t>>(0)), m_alive(std::make_shared<std::atomic<bool>>(true)) {  }

  int m_ID = 0;
  Task m_task;
  std::shared_ptr<std::atomic<int64_t>> m_localQueueSize;
  std::deque<Task> m_localDeque;
  std::shared_ptr<std::atomic<bool>> m_alive;
};

#define LBSPOOL_ENABLE_PROFILE_THREADS
// hosts the worker threads for lbs_awaitable and works as the backend
class LBSPool
{
  struct AllThreadData 
  {
    AllThreadData(int i):data(i) {}
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

  inline void notifyAll(int whoNotified, bool ignoreTasks = false)
  {
    //HIGAN_CPU_FUNCTION_SCOPE();
    auto tasks = std::max(1, idle_threads - tasks_to_do.load());

    for (auto& it : m_threadData)
    {
      if (tasks > 0 && it->data.m_ID != whoNotified && it->data.m_ID != 0)
      {
        tasks--;
        it->cv.notify_one();
      }
      if (ignoreTasks && it->data.m_ID != 0)
        it->cv.notify_one();
    }
  }
  public:
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
    int procs = std::thread::hardware_concurrency();
    // control the amount of threads made.
    //procs = 2;
    for (int i = 0; i < procs; i++)
    {
      m_threadData.emplace_back(std::make_shared<AllThreadData>(i));
    }
    for (auto&& it : m_threadData)
    {
      if (it->data.m_ID == 0) // to let the creator be one of the threads.
        continue; 
      m_threads.push_back(std::thread(&LBSPool::loop, this, it->data.m_ID));
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
  }

  inline bool stealOrSleep(AllThreadData& data, bool allowedToSleep = true) {
    //HIGAN_CPU_FUNCTION_SCOPE();
    // check my own deque
    auto& p = data.data;
    if (p.m_localQueueSize->load() != 0)
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
    // check others deque
    for (auto &it : m_threadData)
    {
      if (it->data.m_ID == p.m_ID) // skip itself
        continue;
      if (it->data.m_localQueueSize->load() != 0) // this should reduce unnecessary lock_guards, and cheap.
      {
        std::lock_guard<std::mutex> guard(it->mutex);
        if (!it->data.m_localDeque.empty()) // double check as it might be empty now.
        {
          p.m_task = it->data.m_localDeque.front();
          assert(p.m_task.m_iterations != 0);
          it->data.m_localDeque.pop_front();
          it->data.m_localQueueSize->store(it->data.m_localDeque.size(), std::memory_order_relaxed);
          tasks_to_do--;
          return true;
        }
      }
    }
    // if all else fails, wait for more work.
    if (!StopCondition && allowedToSleep) // this probably doesn't fix the random deadlock
    {
      if (p.m_localQueueSize->load() != 0)
      {
        return false;
      }
      if (true || idle_threads.fetch_add(1) == threads_awake)
      {
        idle_threads.fetch_sub(1);
        //HIGAN_CPU_BRACKET("short sleeping");
        //std::this_thread::sleep_for(std::chrono::microseconds(10));
        return false;
      }
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
  void informTaskFinished(AllThreadData& data)
  {
#if defined(LBSPOOL_ENABLE_PROFILE_THREADS)
    HIGAN_CPU_FUNCTION_SCOPE();
#endif
    data.data.m_task.m_id = 0;
    std::vector<Task>& addable = data.addableTasks;
    addable.clear();
    {
      BarrierObserver observs = data.data.m_task.m_blocks;
      data.data.m_task.m_blocks = Barrier();
      if (observs.done())
      {
#if defined(LBSPOOL_ENABLE_PROFILE_THREADS)
        HIGAN_CPU_BRACKET("getting task from global queue");
#endif
        std::lock_guard<std::mutex> guard(m_globalWorkMutex);
        bool cond = true;
        while (cond)
        {
          cond = false;
          for (auto it = m_taskQueue.begin(); it != m_taskQueue.end(); it++)
          {
            if (allDepsDone(it->first))
            {
              addable.push_back(std::move(it->second));
              m_taskQueue.erase(it);
              cond = true;
              break;
            }
          }
        }
        tasks_in_queue -= static_cast<int>(addable.size());
      }
    }
    {
#if defined(LBSPOOL_ENABLE_PROFILE_THREADS)
      HIGAN_CPU_BRACKET("adding tasks to own queue");
#endif
      std::lock_guard<std::mutex> guard(data.mutex);
      for (auto& it : addable)
      {
        data.data.m_localDeque.push_front(std::move(it));
        data.data.m_localQueueSize->store(data.data.m_localDeque.size(), std::memory_order_relaxed);
      }
      tasks_to_do += static_cast<int>(addable.size());
      notifyAll(data.data.m_ID);
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

  bool allDepsDone(const std::vector<BarrierObserver>& barrs) {
    for (const auto& barrier : barrs) {
      if (!barrier.done())
        return false;
    }
    return true;
  }

  template<typename Func>
  inline BarrierObserver task(std::vector<BarrierObserver> depends, Func&& func) {
    return internalAddTask<1>(0, depends, 0, 1, std::forward<Func>(func));
  }

  template<size_t ppt, typename Func>
  inline BarrierObserver internalAddTask(int ThreadID, std::vector<BarrierObserver> depends, size_t start_iter, size_t iterations, Func&& func)
  {
#if defined(LBSPOOL_ENABLE_PROFILE_THREADS)
    HIGAN_CPU_FUNCTION_SCOPE();
#endif
    // need a temporary storage for tasks that haven't had requirements filled
    ThreadID = std::max(0, std::min(static_cast<int>(m_threadData.size())-1, ThreadID));
    size_t newId = m_nextTaskID.fetch_add(1);
    assert(newId < m_nextTaskID);
    //printf("task added %zd\n", newId);
    Barrier taskBarrier(newId);
    Task newTask(newId, start_iter, iterations, taskBarrier);
    auto& threadData = m_threadData.at(ThreadID);
    newTask.genWorkFunc<ppt>(std::forward<Func>(func));
    {
      if (allDepsDone(depends))
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
    return BarrierObserver(taskBarrier);
  }

  void helpTasksUntilBarrierComplete(BarrierObserver observed) {
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


static std::shared_ptr<LBSPool> my_pool = nullptr;

template<typename T>
class lbs_awaitable {
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
    auto await_transform(lbs_awaitable<T> handle) {
      //printf("await_transform\n");
      return handle;
    }
    T m_value;
    BarrierObserver dependency;
    Barrier wholeCoroReady;
    std::weak_ptr<coro_handle> weakref;
  };
  using coro_handle = std::experimental::coroutine_handle<promise_type>;
  lbs_awaitable(coro_handle handle) : handle_(std::shared_ptr<coro_handle>(new coro_handle(handle), [](coro_handle* ptr){ptr->destroy();delete ptr;}))
  {
    //printf("created async_awaitable\n");
    assert(handle);

    auto& barrier = observer();
    handle_->promise().wholeCoroReady = Barrier(0);
    handle_->promise().weakref = handle_;

    barrier = my_pool->task({}, [handlePtr = handle_](size_t) mutable {
      if (!handlePtr->done()) {
        //printf("trying to work\n");
        handlePtr->resume();
        if (handlePtr->done()) {
          handlePtr->promise().wholeCoroReady.kill();
        }
      }
    });
  }
  lbs_awaitable(lbs_awaitable& other) {
    handle_ = other.handle_;
  };
  lbs_awaitable(lbs_awaitable&& other) {
    if (other.handle_)
      handle_ = std::move(other.handle_);
    //printf("moving\n");
    assert(handle_);
    other.handle_ = nullptr;
  }
  /*
  bool launch_work_if_work_left() noexcept {
    auto& barrier = observer();
    while (barrier && !barrier.done());
    bool allWorkDone = handle_->done();
    if (!allWorkDone) {
      barrier = my_pool->task({}, [handlePtr = handle_](size_t){
        if (!handlePtr->done()) {
          printf("trying to work\n");
          handlePtr->resume();
        }
      });
    }
    return !allWorkDone;
  }*/
  // coroutine meat
  T await_resume() noexcept {
    //printf("await_resume\n");
    //assert(observer().done());
    //while(launch_work_if_work_left());
    return handle_->promise().m_value;
  }
  bool await_ready() noexcept {
    //printf("await_ready\n");
    auto& barrier = observer();
    //while (barrier && !barrier.done());
    return barrier.done() && handle_->done();
  }

  // enemy coroutine needs this coroutines result, therefore we compute it.
  void await_suspend(coro_handle handle) noexcept {
    //printf("await_suspend\n");
    auto& barrier = observer();

    auto& enemyBarrier = handle.promise().dependency;
    std::shared_ptr<coro_handle> otherHandle = handle.promise().weakref.lock();
    enemyBarrier = my_pool->task({handle_->promise().wholeCoroReady,barrier, enemyBarrier}, [handlePtr = otherHandle](size_t) mutable {
      if (!handlePtr->done()) {
        //printf("trying to do dependant work\n");
        handlePtr->resume();
        if (handlePtr->done())
        {
          handlePtr->promise().wholeCoroReady.kill();
        }
      }
    });
    handle.promise().dependency = enemyBarrier;
  }
  // :o
  ~lbs_awaitable() {
  }

  T get()
  {
    //printf("get\n");
    BarrierObserver obs(handle_->promise().wholeCoroReady);
    //my_pool->helpTasksUntilBarrierComplete(barrier);
    while(!obs.done());
    assert(handle_->done());
    return handle_->promise().m_value; 
  }
  // unwrap() future<future<int>> -> future<int>
  // future then(lambda) -> attach function to be done after current task.
  // is_ready() are you ready?
  coro_handle coro() { return *handle_; }
private:
  BarrierObserver& observer() const { return handle_->promise().dependency;}
  std::shared_ptr<coro_handle> handle_;
};

lbs_awaitable<int> funfun5() {
  co_return 1;
}

lbs_awaitable<int> funfun6() {
  int sum = 0;
  sum += co_await funfun5();
  //printf("wtf!\n");
  co_return sum;
}

lbs_awaitable<int> addInTreeLBS(int treeDepth, int parallelDepth) {
  if (treeDepth <= 0)
    co_return 1;
  if (treeDepth > parallelDepth) {
    int result = 0;
    auto res0 = addInTreeLBS(treeDepth-1, parallelDepth);
    auto res1 = addInTreeLBS(treeDepth-1, parallelDepth);
    //co_await combine(res0, res1);
    //result += res0;
    //result += res1;
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

TEST_CASE("threaded awaitable - lbs")
{
  {
    HIGAN_CPU_BRACKET("threaded awaitable - lbs");
    my_pool = std::make_shared<LBSPool>();
    //auto fut = funfun5().get();
    //fut = funfun6().get();
    //REQUIRE(fut == 1);
    int treeSize = 20;
    int computeTree = 6;
    /*
    for (int i = 0; i < 1000*1000; i++) {
      my_pool->resetIDs();
      if (i % 100 == 0) {
        printf("done %d\n",i);
        fflush(stdout);
      }
      int lbs = addInTreeLBS(treeSize, treeSize-computeTree).get();
      int basecase = addInTreeNormal(treeSize);
      REQUIRE(basecase == lbs);
    }*/
    
    higanbana::Timer time;
    int basecase = addInTreeNormal(treeSize);
    auto normalTime = time.reset();
    int asynced = addInTreeAsync3(treeSize, treeSize-computeTree).get();
    time.reset();
    asynced = addInTreeAsync3(treeSize, treeSize-computeTree).get();
    auto asyncTime = time.reset();
    REQUIRE(basecase == asynced);

    auto lbsed = addInTreeLBS(treeSize, treeSize-computeTree).get();
    REQUIRE(basecase == lbsed);
    auto lbsTime = time.reset();
    printf("normal %.3fms awaitable %.3fms %.2f speedup\n", normalTime/1000.f/1000.f, asyncTime/1000.f/1000.f, float(normalTime)/float(asyncTime));
    printf("normal %.3fms lbs %.3fms %.2f speedup\n", normalTime/1000.f/1000.f, lbsTime/1000.f/1000.f, float(normalTime)/float(lbsTime));
    
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
  higanbana::FileSystem fs("/../../tests/data/");
  higanbana::profiling::writeProfilingData(fs);
}
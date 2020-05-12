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
  //size_t taskid = 0;
  public:
  Barrier(int taskid = 0) : m_counter(std::make_shared<std::atomic<int64_t>>(1))//, taskid(taskid)
  {
  }
  Barrier(const Barrier& copy){
    m_counter = copy.m_counter;
    //taskid = copy.taskid;
    m_counter->fetch_add(1);
  }
  Barrier(Barrier&& other) : m_counter(std::move(other.m_counter))//, taskid(other.taskid)
  {
    other.m_counter = nullptr;
  }
  Barrier& operator=(const Barrier& copy) {
    m_counter->fetch_sub(1);
    m_counter = copy.m_counter;
    //taskid = copy.taskid;
    m_counter->fetch_add(1);
    return *this;
  }
  Barrier& operator=(Barrier&& other) {
    if (m_counter)
      *m_counter -= 1;
    m_counter = std::move(other.m_counter);
    //taskid = other.taskid;
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
  //size_t taskid = 0;
  public:
  BarrierObserver() : m_counter(std::make_shared<std::atomic<int64_t>>(0))
  {
  }
  BarrierObserver(Barrier barrier) : m_counter(barrier.m_counter)//, taskid(barrier.taskid)
  {
  }
  BarrierObserver(const BarrierObserver& copy) : m_counter(copy.m_counter)//, taskid(copy.taskid)
  {
  }
  BarrierObserver(BarrierObserver&& other) : m_counter(std::move(other.m_counter))//, taskid(other.taskid) 
  {
    other.m_counter = nullptr;
  }
  BarrierObserver& operator=(const BarrierObserver& copy) {
    m_counter = copy.m_counter;
    //taskid = copy.taskid;
    return *this;
  }
  BarrierObserver& operator=(BarrierObserver&& other) {
    m_counter = std::move(other.m_counter);
    //taskid = other.taskid;
    other.m_counter = nullptr;
    return *this;
  }
  Barrier barrier() {
    Barrier bar;
    m_counter->fetch_add(1);
    bar.m_counter = m_counter;
    //bar.taskid = taskid;
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
  ThreadData() :m_ID(0), m_task(Task()), m_localQueueSize(std::make_shared<std::atomic<int>>(0)), m_alive(std::make_shared<std::atomic<bool>>(true)) { }
  ThreadData(int id) :m_ID(id), m_task(Task()), m_localQueueSize(std::make_shared<std::atomic<int>>(0)), m_alive(std::make_shared<std::atomic<bool>>(true)) {  }

  int m_ID = 0;
  Task m_task;
  std::shared_ptr<std::atomic<int>> m_localQueueSize;
  std::deque<Task> m_localDeque;
  std::shared_ptr<std::atomic<bool>> m_alive;
};

#define LBSPOOL_ENABLE_PROFILE_THREADS
// hosts the worker threads for lbs_awaitable and works as the backend
thread_local bool thread_from_pool = false;
thread_local int thread_id = -1;
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

  void notifyAll(int whoNotified, bool ignoreTasks = false)
  {
    //HIGAN_CPU_FUNCTION_SCOPE();
    auto tasks = threads_awake.load(); //std::max(1, idle_threads - tasks_to_do.load());

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
    int procs = std::thread::hardware_concurrency();
    // control the amount of threads made.
    // procs = 2;
    for (int i = 0; i < procs; i++)
    {
      m_threadData.emplace_back(std::make_shared<AllThreadData>(i));
    }
    for (auto&& it : m_threadData)
    {
      if (it->data.m_ID == 0) // Basically mainthread is one of *us*
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
      if (p.m_localQueueSize->load() != 0 || tasks_to_do > 0 || tasks_in_queue > 5)
      {
        return false;
      }
      //return false;
      // potentially keep 1 thread always alive? for responsiveness? I don't know.
      /*if (idle_threads.fetch_add(1) >= threads_awake)
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
    
    {
        HIGAN_CPU_BRACKET("dowork");
        rdy = p.m_task.doWork();
    }
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

  template<typename Func>
  inline BarrierObserver task(std::vector<BarrierObserver>&& depends, Func&& func) {
    int queue_count =  my_queue_count();
    bool depsDone = allDepsDone(depends);
    bool spawnTask = !depends.empty() || !depsDone || queue_count <= 1;
    //tasksReadyToProcess() < threads_awake.load(std::memory_order::relaxed)
    spawnTask = !((depends.empty() || depsDone) && (queue_count > 0 && tasks_to_do > threads_awake/2));
    //.if (tasksDone() % 1000 == 0)
    //    printf("tasks done %zu\n", tasksDone());
    if (spawnTask)
      return internalAddTask<1>(std::forward<decltype(depends)>(depends), 0, 1, std::forward<Func>(func));
    else
    {
      func(0);
      return BarrierObserver();
    }
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
    Barrier taskBarrier(newId);
    Task newTask(newId, start_iter, iterations, taskBarrier);
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
      return coro_handle::from_promise(*this);
    }
    auto initial_suspend() {
      return suspend_always();
    }
    auto final_suspend() noexcept {
      return suspend_always();
    }
    void return_value(T value) {m_value = std::move(value);}
    auto yield_value(T value) {
      m_value = std::move(value);
      return std::experimental::suspend_always{};
    }
    void unhandled_exception() {
      std::terminate();
    }
    /*
    auto await_transform(lbs_awaitable<T> handle) {
      return handle;
    }*/
    T m_value;
    BarrierObserver dependency;
    Barrier wholeCoroReady;
    std::weak_ptr<coro_handle> weakref;
  };
  using coro_handle = std::experimental::coroutine_handle<promise_type>;
  lbs_awaitable(coro_handle handle) : handle_(std::shared_ptr<coro_handle>(new coro_handle(handle), [](coro_handle* ptr){ptr->destroy();delete ptr;}))
  {
    HIGAN_CPU_FUNCTION_SCOPE();
    assert(handle);

    auto& barrier = observer();
    handle_->promise().wholeCoroReady = Barrier(0);
    handle_->promise().weakref = handle_;


    barrier = my_pool->task({}, [handlePtr = handle_](size_t) {
      HIGAN_CPU_FUNCTION_SCOPE();
      if (!handlePtr->done()) {
        handlePtr->resume();
        if (handlePtr->done()) {
          handlePtr->promise().wholeCoroReady.kill();
        }
      }
    });
    HIGAN_CPU_BRACKET("end ctr");
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

  // coroutine meat
  T await_resume() noexcept {
    return handle_->promise().m_value;
  }
  bool await_ready() noexcept {
    BarrierObserver obs(handle_->promise().wholeCoroReady);
    return obs.done() && handle_->done();
  }

  // enemy coroutine needs this coroutines result, therefore we compute it.
  void await_suspend(coro_handle handle) noexcept {
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
  }
  // :o
  ~lbs_awaitable() {
  }

  T get()
  {
    BarrierObserver obs(handle_->promise().wholeCoroReady);
    // not safe to call this, should be never called from within coroutined function.
    my_pool->helpTasksUntilBarrierComplete(obs);
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
        co_return Fibonacci(number - 1) + Fibonacci(number - 2);
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
    uint64_t parallel = 7;
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
        return FibonacciCoro(36,36-parallel-3).get();
    };

    BENCHMARK("Fibonacci 38") {
        return FibonacciOrig(38).get();
    };
    BENCHMARK("Coroutine Fibonacci 38") {
        return FibonacciCoro(38,38-parallel-6).get();
    };
    

  //higanbana::FileSystem fs("/../../tests/data/");
  //higanbana::profiling::writeProfilingData(fs);
}
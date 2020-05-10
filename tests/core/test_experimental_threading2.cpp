#include <catch2/catch.hpp>

#include <higanbana/core/system/time.hpp>

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
    genWorkFunc<1>([](size_t) {});
  };
  Task(size_t id, size_t start, size_t iterations) :
    m_id(id),
    m_iterations(iterations),
    m_iterID(start),
    m_ppt(1),
    m_sharedWorkCounter(std::shared_ptr<std::atomic<size_t>>(new std::atomic<size_t>()))
  {
    genWorkFunc<1>([](size_t) {});
    m_sharedWorkCounter->store(m_iterations);
  };

private:
  Task(size_t id, size_t start, size_t iterations, std::shared_ptr < std::atomic<size_t> > sharedWorkCount) :
    m_id(id),
    m_iterations(iterations),
    m_iterID(start),
    m_ppt(1),
    m_sharedWorkCounter(sharedWorkCount)
  {
    genWorkFunc<1>([](size_t) {});
  };
public:

  size_t m_id;
  size_t m_iterations;
  size_t m_iterID;
  int m_ppt;
  bool m_reschedule = false;
  std::shared_ptr < std::atomic<size_t> > m_sharedWorkCounter;

  std::function<bool(size_t&, size_t&)> f_work;

  // Generates ppt sized for -loop lambda inside this work.
  template<size_t ppt, typename Func>
  void genWorkFunc(Func&& func)
  {
    m_ppt = ppt;
    f_work = [func](size_t& iterID, size_t& iterations) -> bool
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
    Task splittedWork(m_id, newStart, iters + m_iterations % 2, m_sharedWorkCounter);
    splittedWork.f_work = f_work;
    m_iterations = iters;
    return splittedWork;
  }
};

class ThreadData
{
public:
  ThreadData() :m_ID(0), m_task(Task()), m_localQueueSize(std::make_shared<std::atomic<int64_t>>(0)) { }
  ThreadData(int id) :m_ID(id), m_task(Task()), m_localQueueSize(std::make_shared<std::atomic<int64_t>>(0)) {  }

  ThreadData(const ThreadData&) = delete;
  ThreadData(ThreadData&&) = default;

  ThreadData& operator=(const ThreadData&) = delete;
  ThreadData& operator=(ThreadData&&) = default;

  int m_ID = 0;
  Task m_task;
  std::shared_ptr<std::atomic<int64_t>> m_localQueueSize;
  std::deque< Task > m_localDeque;
};


// hosts the worker threads for lbs_awaitable and works as the backend
class LBSPool
{
  std::vector< ThreadData >                    m_allThreads;
  std::vector< std::thread >                   m_threads;
  public:
};

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
  };
  using coro_handle = std::experimental::coroutine_handle<promise_type>;
  lbs_awaitable(coro_handle handle) : handle_(std::make_shared<coro_handle>(handle))
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
  lbs_awaitable(lbs_awaitable& other) = delete;
  lbs_awaitable(lbs_awaitable&& other) : handle_(std::move(other.handle_)), m_fut(std::move(other.m_fut)) {
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
  ~lbs_awaitable() { if (handle_) handle_->destroy(); }
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

lbs_awaitable<int> funfun5() {
  co_return 1;
}

lbs_awaitable<int> funfun6() {
  int sum = 0;
  sum += co_await funfun5();
  co_return sum;
}

lbs_awaitable<int> addInTreeLBS(int treeDepth) {
  if (treeDepth <= 0)
    co_return 1;
  if (treeDepth > 20) {
    int result = 0;
    auto res0 = addInTreeLBS(treeDepth-1);
    auto res1 = addInTreeLBS(treeDepth-1);
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

TEST_CASE("threaded awaitable - lbs")
{
  auto fut = funfun5().get();
  fut = funfun6().get();
  REQUIRE(fut == 1);
}
#define CATCH_CONFIG_DISABLE_EXCEPTIONS
#include <catch2/catch.hpp>

#include <higanbana/core/system/time.hpp>
#include <higanbana/core/profiling/profiling.hpp>

#include <higanbana/core/coroutine/task.hpp>
#include <higanbana/core/coroutine/task_stealing_pool.hpp>
#include <higanbana/core/coroutine/stolen_task.hpp>
#include <higanbana/core/coroutine/task_st.hpp>

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

higanbana::coro::Task<int> funfun5() noexcept {
  co_return 1;
}

higanbana::coro::Task<int> funfun6() noexcept {
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

higanbana::coro::Task<int> addInTreeLBS(int treeDepth, int parallelDepth) noexcept {
  if (treeDepth <= 0)
    co_return 1;
  if (treeDepth > parallelDepth) {
    int result = 0;
    auto res0 = addInTreeLBS(treeDepth-1, parallelDepth);
    auto res1 = addInTreeLBS(treeDepth-1, parallelDepth);
    result += co_await res0;
    result += co_await res1;
    co_return result;
  }
  else {
    auto res0 = addInTreeNormal(treeDepth-1);
    auto res1 = addInTreeNormal(treeDepth-1);
    co_return res0 + res1;
  }
}
higanbana::coro::StolenTask<int> addInTreeTS(int treeDepth, int parallelDepth) noexcept {
  if (treeDepth <= 0)
    co_return 1;
  if (treeDepth > parallelDepth) {
    int result = 0;
    auto res0 = addInTreeTS(treeDepth-1, parallelDepth);
    auto res1 = addInTreeTS(treeDepth-1, parallelDepth);
    result += co_await res0;
    result += co_await res1;
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
higanbana::coro::Task<uint64_t> FibonacciCoro(uint64_t number, uint64_t parallel) noexcept {
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
higanbana::coro::StolenTask<uint64_t> FibonacciCoroS(uint64_t number, uint64_t parallel) noexcept {
  if (number < 2)
      co_return 1;
  
  if (number > parallel) {
      auto v0 = FibonacciCoroS(number-1, parallel);
      auto v1 = FibonacciCoroS(number-2, parallel);
      co_return co_await v0 + co_await v1;
  }
  auto fib0 = Fibonacci(number - 1);
  auto fib1 = Fibonacci(number - 2);
  co_return fib0 + fib1;
}


higanbana::coro::StolenTask<int> asyncLoopTest(int treeSize, int computeTree, size_t compareTime) noexcept {
  higanbana::Timer time2;

  size_t mint = 0, maxt = 0;
  size_t avegMin = 0, avegMax = 0;
  size_t aveCount = 0;
  int a = addInTreeNormal(treeSize);
  compareTime = time2.reset();
  mint = compareTime;
  maxt = 0;
  auto omi = mint;
  auto oma = 0;
  //auto overlap = addInTreeLBS(treeSize, treeSize-computeTree);
  size_t aveg = 0;
  for (int i = 0; i < 2000; i++) {
    //my_pool->resetIDs();
    //if (i % 100 == 0) {

    //}
    //funfun6().get();
    //printf("start work\n");
    auto another = addInTreeTS(treeSize, treeSize-computeTree);
    //printf("doing work\n");//for (auto &it : m_threadData)
    int lbs = co_await another;//overlap;
    REQUIRE(a == lbs);
    //printf("got result %d\n", lbs);
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
  co_return a; //co_await overlap;
}

higanbana::corost::Task<int> funst() noexcept {
  printf("I was done!\n");
  co_return 1;
}

higanbana::corost::Task<int> funst1() noexcept {
  int sum = 3;
  auto thing = funst();
  auto thing2 = funst();
  auto thing3 = funst();
  //sum += co_await thing;
  //sum += co_await thing2;
  //sum += co_await thing3;
  co_return sum;
}

int funfunst() {
  return funst1().get();
}

TEST_CASE("threaded awaitable - lbs")
{
  {
    HIGAN_CPU_BRACKET("threaded awaitable - lbs");
    int computeTree = 9;
    int treeSize = 26;
    
    int a = addInTreeNormal(treeSize);
    a = addInTreeNormal(treeSize);
    a = addInTreeNormal(treeSize);
    a = addInTreeNormal(treeSize);
    auto time2 = higanbana::Timer();
    a = addInTreeNormal(treeSize);
    auto stTime = time2.reset();
    //higanbana::experimental::globals::createGlobalLBSPool();
    higanbana::taskstealer::globals::createTaskStealingPool();
    //my_pool = std::make_shared<LBSPool>();
    //auto fut = funfun5().get();
    //fut = funfun6().get();
    
    //REQUIRE(fut == 3);
    
    higanbana::experimental::stts::globals::createSingleThreadPool();

    //auto newf = funst1().get();
    //newf = funst.get();
    //REQUIRE(newf == 3);

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
    REQUIRE(a == result);
    
    /*
    time.reset();
    int basecase = addInTreeNormal(treeSize);
    auto normalTime = time.reset();
    int asynced = basecase;//addInTreeAsync3(treeSize, treeSize-computeTree).get();
    time.reset();
    asynced = addInTreeTS(treeSize, treeSize-computeTree).get();
    REQUIRE(basecase == asynced);
    auto asyncTime = time.reset();
    //auto lbsed0 = addInTreeLBS(treeSize, treeSize-computeTree);
    auto lbsed = addInTreeLBS(treeSize, treeSize-computeTree).get();
    //lbsed0.get();
    REQUIRE(basecase == lbsed);
    auto lbsTime = time.reset();
    printf("normal %.3fms awaitable %.3fms %.2f speedup\n", normalTime/1000.f/1000.f, asyncTime/1000.f/1000.f, float(normalTime)/float(asyncTime));
    printf("tasks done %zd normal %.3fms lbs %.3fms %.2f speedup\n", higanbana::experimental::globals::s_pool->tasksDone(), normalTime/1000.f/1000.f, lbsTime/1000.f/1000.f, float(normalTime)/float(lbsTime));
    */
    
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
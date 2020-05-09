#include <catch2/catch.hpp>

#include <higanbana/core/system/time.hpp>

#include <vector>
#include <thread>
#include <future>
#include <optional>
#include <cstdio>
#include <iostream>
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

TEST_CASE("c++ threading")
{
  resumable res = foo();
  std::cout << "created resumable" << std::endl;
  while (res.resume()) {std::cout << "resume coroutines!!" << std::endl;}
}

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
  printf("my_future %.3fms\n", asyncTime2/1000.f/1000.f);
}
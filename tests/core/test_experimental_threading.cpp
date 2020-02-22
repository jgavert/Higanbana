#include <catch2/catch.hpp>

#include <vector>
#include <thread>
#include <future>
#include <optional>
#include <cstdio>

std::shared_future<int> localpass(int i, int seconds)
{
  return std::async([i, seconds](){
    printf("worker: list %d localpass started\n", i);
    std::this_thread::sleep_for(std::chrono::milliseconds(seconds*1));
    printf("worker: list %d localpass ended\n", i);
    return i;
  });
}

std::shared_future<int> globalPass(
  std::optional<std::shared_future<int>> dependantGlobalTask,
  std::shared_future<int> myLocalPass)
{
  return std::async([dependantGlobalTask, myLocalPass]() mutable {
    auto taskIndex = myLocalPass.get();
    auto work = (taskIndex % 2 == 0) * 10 - taskIndex;
    if (dependantGlobalTask)
    {
      auto previousTaskIndex = dependantGlobalTask->get();
      printf("GlobalPass list (%d ->) %d\n", previousTaskIndex, taskIndex);
      return taskIndex;
    }
    printf("GlobalPass list %d\n", taskIndex);
    return taskIndex;
  });
}

std::shared_future<int> fillList(
  std::shared_future<int> myGlobalTask)
{
  return std::async([myGlobalTask]() mutable {
    auto taskIndex = myGlobalTask.get();
    auto work = (taskIndex % 2 == 0) * 10 - taskIndex;
    printf("worker: list %d fillList started\n", taskIndex);
    std::this_thread::sleep_for(std::chrono::milliseconds(work*1));
    printf("worker: list %d fillList ended\n", taskIndex);
    return taskIndex;
  });
}

std::shared_future<int> submitList(
  std::optional<std::shared_future<int>> lastSubmitDone,
  std::shared_future<int> myListFilled)
{
  return std::async([lastSubmitDone, myListFilled]() mutable {
    if (lastSubmitDone)
    {
      auto val = lastSubmitDone->get(); // waiting for last submit
      printf("worker: dependant submit was done (%d)\n", val);
    }
    auto taskIndex = myListFilled.get();
    auto work = (taskIndex % 2 == 0) * 10 - taskIndex;
    printf("worker: submit list %d\n", taskIndex);
    std::this_thread::sleep_for(std::chrono::milliseconds(work*1));
    printf("worker: submit list %d done\n", taskIndex);
    return taskIndex;
  });
}

TEST_CASE("c++ threading")
{
  std::vector<std::shared_future<int>> listsSubmitted;
  std::optional<std::shared_future<int>> lastGlobalPass;
  std::optional<std::shared_future<int>> lastSubmitPass;

  for (int i = 0; i < 3; ++i)
  {
    printf("Issued Task %d\n", i);
    auto globalDone = globalPass(lastGlobalPass, localpass(i, 1));
    lastGlobalPass = globalDone;
    auto submitDone = submitList(lastSubmitPass, fillList(globalDone));
    lastSubmitPass = submitDone;
    listsSubmitted.emplace_back(submitDone);
  }
  int i = 0;
  for (auto&& task : listsSubmitted)
  {
    auto val = task.get();
    printf("Finish %d!\n", val);
    REQUIRE(i == val);
    i++;
  }
}
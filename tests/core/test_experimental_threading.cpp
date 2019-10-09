#include <catch2/catch.hpp>

#include <vector>
#include <thread>
#include <future>
#include <optional>
#include <cstdio>

std::shared_future<int> do_work(int i, int seconds)
{
  return std::async([i, seconds](){
    printf("worker: Task %d Started\n", i);
    std::this_thread::sleep_for(std::chrono::milliseconds(seconds*1));
    printf("worker: Task %d Ended\n", i);
    return i;
  });
}

std::shared_future<std::pair<int, std::shared_future<int>>> do_ordered_work(
  std::optional<std::shared_future<std::pair<int, std::shared_future<int>>>> dependantGlobalTask,
  std::shared_future<int> dependantWork)
{
  return std::async([dependantGlobalTask, dependantWork]() mutable {
    auto taskIndex = dependantWork.get();
    auto work = (taskIndex % 2 == 0) * 10 - taskIndex;
    if (dependantGlobalTask)
    {
      auto previousTaskIndex = dependantGlobalTask->get().first;
      printf("Global Task %d -> %d\n", previousTaskIndex, taskIndex);
      return std::make_pair(taskIndex, do_work(taskIndex, work));
    }
    printf("Global Task %d\n", taskIndex);
    return std::make_pair(taskIndex, do_work(taskIndex, work));
  });
}

TEST_CASE("c++ threading")
{
  std::vector<std::shared_future<std::pair<int, std::shared_future<int>>>> finalTasks;
  std::optional<std::shared_future<std::pair<int, std::shared_future<int>>>> lastIssued;

  for (int i = 0; i < 6; ++i)
  {
    printf("Issued Task %d\n", i);
    auto thisWork = do_ordered_work(lastIssued, do_work(i, 10-i));
    finalTasks.emplace_back(thisWork);
    lastIssued = thisWork;
  }
  int i = 0;
  for (auto&& task : finalTasks)
  {
    auto val = task.get().second.get();
    printf("Finish %d!\n", val);
    REQUIRE(i == val);
    i++;
  }
}
#include "schedulertests.hpp"
#include "TestWorks.hpp"
using namespace faze;
bool SchedulerTests::Run()
{
	TestWorks t("SchedulerTests");

	t.addTest("Run a Task", []()
	{
		LBS s(2);
		s.addTask("task", [](size_t, size_t)
			{

			});
		s.sleepTillKeywords({"task"});
		return true;
	});
	t.addTest("mutate something in a task", []()
	{
		LBS s(2);
		int a = 0;
		s.addTask("task", [&a](size_t, size_t)
			{
				a = 1;
			});
		s.sleepTillKeywords({"task"});
		return (a == 1);
	});
	t.addTest("ParallelFor", []()
	{
		LBS s(5);
		std::vector<int> v;
    v.reserve(100000);
		for (int i=0; i<100000;i++)
		{
			v.emplace_back(i);
		}
		s.addParallelFor<32>("task", {}, {}, 0, 100000, [&v](size_t i, size_t)
			{
				v[i] = 0;
			});
		s.sleepTillKeywords({"task"});
		for (auto&& it : v)
		{
			if (it != 0)
				return false;
		}
		return true;
	});

  t.addTest("ParallelFor2", []()
  {
    LBS s;
    std::vector<int> v;
    v.reserve(2000000);
    for (int i = 0; i < 2000000;i++)
    {
      v.emplace_back(i);
    }
    s.addParallelFor<1024>("task", {}, {}, 0, 2000000, [&v](size_t i, size_t)
    {
      v[i] = 0;
    });
    s.sleepTillKeywords({ "task" });
    for (auto&& it : v)
    {
      if (it != 0)
        return false;
    }
    return true;
  });

  t.addTest("ParallelFor3", []()
  {
    LBS s;
    std::unique_ptr<std::array<int, 2000000>> v = std::make_unique<std::array<int, 2000000>>();
    std::atomic<bool> b(true);

    s.addParallelFor<1024>("task", {}, {}, 0, 2000000, [&v](size_t i, size_t)
    {
      (*v)[i] = i;
    });
    s.addParallelFor<1024>("task2", {"task"}, {}, 0, 2000000, [&v, &b](size_t i, size_t)
    {
      if ((*v)[i] != i)
        b = false;
    });
    s.sleepTillKeywords({ "task2" });
    return b.load();
  });

	t.addTest("Sequential tasks", []()
	{
		LBS s(2);
		std::vector<int> v;
		s.addTask("task5", {"task4"}, {}, [&v](size_t, size_t)
		{
			v.emplace_back(5);
		});
		s.addTask("task3", {"task2"}, {}, [&v](size_t, size_t)
		{
			v.emplace_back(3);
		});
		s.addTask("task4", {"task3"}, {}, [&v](size_t, size_t)
		{
			v.emplace_back(4);
		});
		s.addTask("task1", {}, {}, [&v](size_t, size_t)
		{
			v.emplace_back(1);
		});
		s.addTask("task2", {"task1"}, {}, [&v](size_t, size_t)
		{
			v.emplace_back(2);
		});
		s.sleepTillKeywords({/*"task1", "task2", "task3", "task4",*/ "task5"});
		if (v.size() != 5)
			return false;
		for (int i=1; i<6; i++)
		{
			if (v[i-1] != i)
				return false;
		}
		return true;
	});
	return t.runTests();
}
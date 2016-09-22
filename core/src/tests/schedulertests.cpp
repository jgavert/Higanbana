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
		s.sleepTillKeywords({ "task" });
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
		s.sleepTillKeywords({ "task" });
		return (a == 1);
	});
	t.addTest("ParallelFor<32>", []()
	{
		LBS s(5);
		std::vector<int> v;
		constexpr size_t testSize = 1000000;
		v.reserve(testSize);
		for (int i = 0; i < testSize; i++)
		{
			v.emplace_back(i);
		}
		s.addParallelFor<32>("task", {}, {}, 0, testSize, [&v](size_t i, size_t)
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

	t.addTest("ParallelFor<1024>", []()
	{
		LBS s;
		std::vector<int> v;
		constexpr size_t testSize = 1000000;
		v.reserve(testSize);
		for (int i = 0; i < testSize; i++)
		{
			v.emplace_back(i);
		}
		s.addParallelFor<1024>("task", {}, {}, 0, testSize, [&v](size_t i, size_t)
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

	t.addTest("ParallelFor<8192>", []()
	{
		LBS s;
		std::vector<int> v;
		constexpr size_t testSize = 1000000;
		v.reserve(testSize);
		for (int i = 0; i < testSize; i++)
		{
			v.emplace_back(i);
		}
		s.addParallelFor<8192>("task", {}, {}, 0, testSize, [&v](size_t i, size_t)
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

	t.addTest("ParallelFor2", []()
	{
		LBS s;
		constexpr size_t testSize = 1000000;
		std::unique_ptr<size_t[]> v = std::unique_ptr<size_t[]>(new size_t[testSize]);
		std::atomic<bool> b(true);

		for (int i = 0; i < testSize; i++)
		{
			v[i] = i;
		}
		s.addParallelFor<1024>("task", { }, {}, 0, testSize, [&v, &b](size_t i, size_t)
		{
			if (v[i] != i)
				b = false;
		});
		s.sleepTillKeywords({ "task" });
		return b.load();
	});

	t.addTest("ParallelFor3<32>", []()
	{
		LBS s;
		constexpr size_t testSize = 1000000;
		std::unique_ptr<size_t[]> v = std::unique_ptr<size_t[]>(new size_t[testSize]);
		std::atomic<bool> b(true);

		s.addParallelFor<32>("task", {}, {}, 0, testSize, [&v](size_t i, size_t)
		{
			v[i] = i;
		});
		s.addParallelFor<32>("task2", { "task" }, {}, 0, testSize, [&v, &b](size_t i, size_t)
		{
			if (v[i] != i)
				b = false;
		});
		s.sleepTillKeywords({ "task2" });
		return b.load();
	});

	t.addTest("ParallelFor3<1024>", []()
	{
		LBS s;
		constexpr size_t testSize = 1000000;
		std::unique_ptr<size_t[]> v = std::unique_ptr<size_t[]>(new size_t[testSize]);
		std::atomic<bool> b(true);

		s.addParallelFor<1024>("task", {}, {}, 0, testSize, [&v](size_t i, size_t)
		{
			v[i] = i;
		});
		s.addParallelFor<1024>("task2", { "task" }, {}, 0, testSize, [&v, &b](size_t i, size_t)
		{
			if (v[i] != i)
				b = false;
		});
		s.sleepTillKeywords({ "task2" });
		return b.load();
	});

	t.addTest("ParallelFor3<2048>", []()
	{
		LBS s;
		constexpr size_t testSize = 1000000;
		std::unique_ptr<size_t[]> v = std::unique_ptr<size_t[]>(new size_t[testSize]);
		std::atomic<bool> b(true);

		s.addParallelFor<2048>("task", {}, {}, 0, testSize, [&v](size_t i, size_t)
		{
			v[i] = i;
		});
		s.addParallelFor<2048>("task2", { "task" }, {}, 0, testSize, [&v, &b](size_t i, size_t)
		{
			if (v[i] != i)
				b = false;
		});
		s.sleepTillKeywords({ "task2" });
		return b.load();
	});

	t.addTest("ParallelFor3<8192>", []()
	{
		LBS s;
		constexpr size_t testSize = 1000000;
		std::unique_ptr<size_t[]> v = std::unique_ptr<size_t[]>(new size_t[testSize]);
		std::atomic<bool> b(true);

		s.addParallelFor<8192>("task", {}, {}, 0, testSize, [&v](size_t i, size_t)
		{
			v[i] = i;
		});
		s.addParallelFor<8192>("task2", { "task" }, {}, 0, testSize, [&v, &b](size_t i, size_t)
		{
			if (v[i] != i)
				b = false;
		});
		s.sleepTillKeywords({ "task2" });
		return b.load();
	});

	t.addTest("Sequential tasks", []()
	{
		LBS s(5);
		std::vector<size_t> v;
		s.addTask("task5", { "task4" }, {}, [&v](size_t, size_t)
		{
			v.emplace_back(5);
		});
		s.addTask("task3", { "task2" }, {}, [&v](size_t, size_t)
		{
			v.emplace_back(3);
		});
		s.addTask("task4", { "task3" }, {}, [&v](size_t, size_t)
		{
			v.emplace_back(4);
		});
		s.addTask("task1", {}, {}, [&v](size_t, size_t)
		{
			v.emplace_back(1);
		});
		s.addTask("task2", { "task1" }, {}, [&v](size_t, size_t)
		{
			v.emplace_back(2);
		});
		s.sleepTillKeywords({/*"task1", "task2", "task3", "task4",*/ "task5" });
		if (v.size() != 5)
			return false;
		for (size_t i = 1; i < 6; i++)
		{
			if (v[i - 1] != i)
				return false;
		}
		return true;
	});
	return t.runTests();
}

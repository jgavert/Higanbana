#include "schedulertests.hpp"
#include "TestWorks.hpp"

using namespace faze;

bool SchedulerTests::Run()
{
	TestWorks t("SchedulerTests");
  LBS s;
	t.addTest("Run a Task", [&]()
	{
		s.addTask("task", [](size_t, size_t)
		{

		});
		s.sleepTillKeywords({ "task" });
		return true;
	});
	t.addTest("mutate something in a task", [&]()
	{
		int a = 0;
		s.addTask("task", [&a](size_t, size_t)
		{
			a = 1;
		});
		s.sleepTillKeywords({ "task" });
		return (a == 1);
	});
	t.addTest("ParallelFor<32>", [&]()
	{
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

	t.addTest("ParallelFor<1024>", [&]()
	{
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

	t.addTest("ParallelFor<8192>", [&]()
	{
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

	t.addTest("ParallelFor2", [&]()
	{
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

  t.addTest("ParallelFor3<8>", [&]()
  {
    constexpr size_t testSize = 1000000;
    std::unique_ptr<size_t[]> ptr = std::unique_ptr<size_t[]>(new size_t[testSize]);
    std::atomic<bool> b(true);
    size_t* v = ptr.get();
    s.addParallelFor<8>("task", {}, {}, 0, testSize, [&v](size_t i, size_t)
    {
      v[i] = i;
    });
    s.addParallelFor<8>("task2", { "task" }, {}, 0, testSize, [&v, &b](size_t i, size_t)
    {
      if (v[i] != i)
        b = false;
    });
    s.sleepTillKeywords({ "task2" });
    return b.load();
  });

	t.addTest("ParallelFor3<32>", [&]()
	{
		constexpr size_t testSize = 1000000;
    std::unique_ptr<size_t[]> ptr = std::unique_ptr<size_t[]>(new size_t[testSize]);
    std::atomic<bool> b(true);
    size_t* v = ptr.get();

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

  t.addTest("ParallelFor3<64>", [&]()
  {
    constexpr size_t testSize = 1000000;
    std::unique_ptr<size_t[]> ptr = std::unique_ptr<size_t[]>(new size_t[testSize]);
    std::atomic<bool> b(true);
    size_t* v = ptr.get();

    s.addParallelFor<64>("task", {}, {}, 0, testSize, [&v](size_t i, size_t)
    {
      v[i] = i;
    });
    s.addParallelFor<64>("task2", { "task" }, {}, 0, testSize, [&v, &b](size_t i, size_t)
    {
      if (v[i] != i)
        b = false;
    });
    s.sleepTillKeywords({ "task2" });
    return b.load();
  });

  t.addTest("ParallelFor3<128>", [&]()
  {
    constexpr size_t testSize = 1000000;
    std::unique_ptr<size_t[]> ptr = std::unique_ptr<size_t[]>(new size_t[testSize]);
    std::atomic<bool> b(true);
    size_t* v = ptr.get();
    s.addParallelFor<128>("task", {}, {}, 0, testSize, [&v](size_t i, size_t)
    {
      v[i] = i;
    });
    s.addParallelFor<128>("task2", { "task" }, {}, 0, testSize, [&v, &b](size_t i, size_t)
    {
      if (v[i] != i)
        b = false;
    });
    s.sleepTillKeywords({ "task2" });
    return b.load();
  });

  t.addTest("ParallelFor3<256>", [&]()
  {
    constexpr size_t testSize = 1000000;
    std::unique_ptr<size_t[]> ptr = std::unique_ptr<size_t[]>(new size_t[testSize]);
    std::atomic<bool> b(true);
    size_t* v = ptr.get();

    s.addParallelFor<256>("task", {}, {}, 0, testSize, [&v](size_t i, size_t)
    {
      v[i] = i;
    });
    s.addParallelFor<256>("task2", { "task" }, {}, 0, testSize, [&v, &b](size_t i, size_t)
    {
      if (v[i] != i)
        b = false;
    });
    s.sleepTillKeywords({ "task2" });
    return b.load();
  });

	t.addTest("ParallelFor3<1024>", [&]()
	{
		constexpr size_t testSize = 1000000;
    std::unique_ptr<size_t[]> ptr = std::unique_ptr<size_t[]>(new size_t[testSize]);
    std::atomic<bool> b(true);
    size_t* v = ptr.get();

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

	t.addTest("ParallelFor3<2048>", [&]()
	{
		constexpr size_t testSize = 1000000;
    std::unique_ptr<size_t[]> ptr = std::unique_ptr<size_t[]>(new size_t[testSize]);
    std::atomic<bool> b(true);
    size_t* v = ptr.get();

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

	t.addTest("ParallelFor3<8192>", [&]()
	{
		constexpr size_t testSize = 1000000;
    std::unique_ptr<size_t[]> ptr = std::unique_ptr<size_t[]>(new size_t[testSize]);
    std::atomic<bool> b(true);
    size_t* v = ptr.get();

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

	t.addTest("Sequential tasks", [&]()
	{
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

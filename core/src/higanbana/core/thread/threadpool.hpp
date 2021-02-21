#pragma once
#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <deque>
#include <iostream>
#include <algorithm>
#include <cassert>
#include <mutex>
#include <condition_variable>

#include "higanbana/core/datastructures/vector.hpp"

// optional include, just dont enable "debug"
#include "higanbana/core/global_debug.hpp"

//#define DEBUG
//#define DEADLOCKCHECK // this should be cheap to keep on, not

#if defined(HIGANBANA_PLATFORM_WINDOWS)
#if 0
#undef PROFILE
#define PROFILE
#include <WinPixEventRuntime/pix3.h>
#undef PROFILE
#endif
#endif

namespace higanbana
{
  struct Requirements
  {
    std::vector<std::string> m_regs;
    //Requirements(Requirements&& tmp):m_regs(tmp.m_regs){}
    /*
    template<typename... Args>
    Requirements(Args&&... args)
      : m_regs(std::forward<Args>(args)...)
    {  }
    */
    Requirements(std::initializer_list<std::string>&& args)
      : m_regs(std::forward<std::initializer_list<std::string>>(args))
    {
    }

    Requirements(std::vector<std::string>&& args)
      : m_regs(std::forward<std::vector<std::string>>(args))
    {
    }
  };

  struct TaskInfo
  {
    TaskInfo() :m_post({}) {}
    TaskInfo(std::string name, Requirements post) : m_name(name), m_post(post) {}
    std::string m_name;
    Requirements m_post;
  };

  static thread_local int t_threadid;
  static thread_local bool t_reSchedule;

  inline void rescheduleTask()
  {
    //HIGAN_LOG("rescheduled in thread %d\n", t_threadid);
    t_reSchedule = true;
  }

  class Task /*TaskDescriptor*/
  {
  public:
    Task() :
      m_id(0),
      m_iterations(0),
      m_iterID(0),
      m_originalIterations(m_iterations),
      m_originalIterID(m_iterID),
      m_ppt(1),
      m_sharedWorkCounter(std::shared_ptr<std::atomic<size_t>>(new std::atomic<size_t>()))
    {
      genWorkFunc<1>([](size_t) {});
    };
    Task(size_t id, size_t start, size_t iterations) :
      m_id(id),
      m_iterations(iterations),
      m_iterID(start),
      m_originalIterations(m_iterations),
      m_originalIterID(m_iterID),
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
      m_originalIterations(m_iterations),
      m_originalIterID(m_iterID),
      m_ppt(1),
      m_sharedWorkCounter(sharedWorkCount)
    {
      genWorkFunc<1>([](size_t) {});
    };
  public:

    size_t m_id;
    size_t m_iterations;
    size_t m_iterID;
    size_t m_originalIterations;
    size_t m_originalIterID;
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

    // does ppt amount of work
    inline bool doWork()
    {
      return f_work(m_iterID, m_iterations);
    }

    inline bool canSplit()
    {
      return (m_iterations > static_cast<size_t>(m_ppt));
    }

    // It is decided to split, this accomplishes that part.
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

    //void setGlobalID() { G_ID = m_ID;}
    int m_ID = 0;
    Task m_task;
    std::shared_ptr<std::atomic<int64_t>> m_localQueueSize;
    std::deque< Task > m_localDeque;
  };

  namespace desc
  {
    struct Task
    {
      Task() :pre({}), post({}) {}
      Task(std::string name, Requirements pre, Requirements post) :name(name), pre(pre), post(post) {}
      std::string name;
      Requirements pre;
      Requirements post;
    };
  }

  class LBS
  {
  public:
    enum ThreadState
    {
      WAITINGFORWORK,
      WORKING,
      RUNNINGLOGIC
    };
  private:
    // for threads to sleep on same condition variable
    std::condition_variable m_cv;
    std::mutex              m_sleeping;

    // condition variables per thread
    std::vector< std::shared_ptr < std::condition_variable > > m_cvs;

    // Thread related data
    std::vector< ThreadData >                    m_allThreads;
    std::vector< std::thread >                   m_threads;
    std::vector< std::shared_ptr< std::mutex > > m_mutexes;

    // Requirements data
    std::mutex                                            m_wfrMutex;                // only one mutex :D to rule them all
    higanbana::unordered_map< std::string, bool >               m_fullfilled;              // fulfilled Tasks
    higanbana::unordered_map< size_t, TaskInfo >                m_taskInfos;               // take infos from here when task finished
    std::vector< std::pair< Requirements, std::string > > m_waitingPostRequirements; // When requirement is filled -> move string to m_fulfilled
    std::vector< std::pair< Requirements, Task > >         m_waitingPreRequirements;  // Put task in here if preR isn't fulfilled
    //END

    // global condition for making threads quit.
    std::atomic<bool> StopCondition;

    // running id for Tasks
    std::atomic<size_t> m_nextTaskID;

    // Thread status related stuff, probably not needed
    // This could still be used as realtime sampling. Needs a dedicated thread to get good data, otherwise it can only give direction.
    std::atomic< int > idle_threads;
    // TODO: Would be better to be able to "time" how long does it take to complete a task. Visualizing this would be fun.
    // idea: Start counting when a thread finds a "new" task to do, -> inform to start timer
    //       then when the task is "complete" -> inform to stop timer
    struct waiting2
    {
      waiting2() :m(new std::mutex), cv(new std::condition_variable) {}
      std::shared_ptr<std::mutex> m;
      std::shared_ptr<std::condition_variable> cv;
    };
    waiting2 m_waiting;
    std::atomic<bool> m_mainthreadsleeping;
  public:

    LBS()
      : StopCondition(false)
      , m_nextTaskID(1)
      , idle_threads(0)
      , m_mainthreadsleeping(false)
    {
      int procs = std::thread::hardware_concurrency();
      // control the amount of threads made.
      //procs = 1;
      for (int i = 0; i < procs; i++)
      {
        m_allThreads.emplace_back(i);
        ThreadStatus.push_back(std::make_pair(RUNNINGLOGIC, i));
      }
      for (int i = 0; i < static_cast<int>(m_allThreads.size()); i++)
      {
        std::shared_ptr<std::mutex> temp(new std::mutex());
        m_mutexes.push_back(std::move(temp));
        std::shared_ptr<std::condition_variable> temp2(new std::condition_variable());
        m_cvs.push_back(std::move(temp2));
      }
      for (auto&& it : m_allThreads)
      {
        m_threads.push_back(std::thread(&LBS::loop, this, it.m_ID));
      }
#ifdef DEBUGTEXT
      HIGAN_LOG("LBS: Initializing with %d threads\n", procs);
#endif
    }
    LBS(int threadCount)
      : StopCondition(false)
      , m_nextTaskID(1)
      , idle_threads(0)
      , m_mainthreadsleeping(false)
    {
      int procs = threadCount;
      // control the amount of threads made.
      //procs = 1;
      for (int i = 0; i < procs; ++i)
      {
        //ThreadData wtf(i);
        m_allThreads.emplace_back(i);
        ThreadStatus.push_back(std::make_pair(RUNNINGLOGIC, i));
      }
      for (size_t i = 0; i < static_cast<size_t>(procs); ++i)
      {
        std::shared_ptr<std::mutex> temp(new std::mutex());
        m_mutexes.push_back(std::move(temp));
        std::shared_ptr<std::condition_variable> temp2(new std::condition_variable());
        m_cvs.push_back(std::move(temp2));
      }
      for (auto& it : m_allThreads)
      {
        m_threads.push_back(std::thread(&LBS::loop, this, it.m_ID));
      }
#ifdef DEBUGTEXT
      HIGAN_LOG("LBS: Initializing with %d threads\n", procs);
#endif
    }

    ~LBS() // you do not simply delete this
    {
      StopCondition.store(true);
      for (size_t i = 0; i < m_threads.size(); i++)
      {
          addTask(static_cast<int>(i%m_threads.size()), "EndThread", [](size_t) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); return; }); // mm
      }
      for (auto& it : m_threads)
      {
        while (!it.joinable()) { notifyAll(); }
        it.join();
      }
    }

    size_t threadCount() const
    {
      return m_threads.size();
    }

    // Princess sleeps here
    void sleepTillKeywords(Requirements req)
    {
#ifdef DEBUGTEXT
      HIGAN_LOG("T%d: sleepTillKeyword name %s\n", std::this_thread::get_id(), ""/*req.c_str()*/);
#endif
      //notifyAll();
      std::unique_lock<std::mutex> lkk(*m_waiting.m);
      internalAddTask<1>(0, "WakePrincess", req, {}, 0, 1, [&](size_t)
      {
        std::lock_guard<std::mutex> np(*m_waiting.m);
#ifdef DEBUGTEXT
        HIGAN_LOG("T%d: Trying to wake princess\n", std::this_thread::get_id());
#endif
        m_waiting.cv->notify_all();
      });
#ifdef DEBUGTEXT
      HIGAN_LOG("T%d: sleepTillKeyword name %s going to sleep\n", std::this_thread::get_id(), "" /*req.c_str()*/);
#endif
#ifdef DEADLOCKCHECK
      checkDeadlock(-1);
      m_mainthreadsleeping = true;
#endif
      notifyAll();
      m_waiting.cv->wait(lkk);
#ifdef DEADLOCKCHECK
      //checkDeadlock(-1);
      m_mainthreadsleeping = false;
#endif
#ifdef DEBUGTEXT
      HIGAN_LOG("T%d: sleepTillKeyword name %s was woken\n", std::this_thread::get_id(), ""/*req.c_str()*/);
#endif
    }

    template<typename Func>
    void addTask(std::string name, Func&& func)
    {
      internalAddTaskWithoutRequirements<1>(0, name, 0, 1, std::forward<Func>(func));
    }

    template<typename Func>
    void addTask(std::string name, Requirements pre, Requirements post, Func&& func)
    {
      internalAddTask<1>(0, name, pre, post, 0, 1, std::forward<Func>(func));
    }

    template <typename Func>
    void addTask(const desc::Task& desc, Func&& func)
    {
      internalAddTask<1>(0, desc.name, desc.pre, desc.post, 0, 1, std::forward<Func>(func));
    }

    template <size_t size, typename Func>
    void addParallelFor(const desc::Task& desc, size_t start_iter, size_t iterations, Func&& func)
    {
      internalAddTask<size>(0, desc.name, desc.pre, desc.post, start_iter, iterations, std::forward<Func>(func));
    }
    template <size_t size, typename Func>
    void addParallelFor(std::string name, Requirements pre, Requirements post, size_t start_iter, size_t iterations, Func&& func)
    {
      internalAddTask<size>(0, name, pre, post, start_iter, iterations, std::forward<Func>(func));
    }
  private:

    // Informs all the threads about new work.
    inline void notifyAll()
    {
      for (auto& it : m_cvs)
      {
        it->notify_one();
      }
    }

    template<typename Func>
    inline void addTask(int ThreadID, std::string name, Func&& func)
    {
      internalAddTaskWithoutRequirements<1>(ThreadID, name, 0, 1, std::forward<Func>(func));
    }

    template<size_t ppt, typename Func>
    inline void internalAddTaskWithoutRequirements(int ThreadID, std::string name, size_t start_iter, size_t iterations, Func&& func)
    {
      // need a temporary storage for tasks that haven't had requirements filled
      size_t newId = m_nextTaskID.fetch_add(1);
      assert(newId < m_nextTaskID);
#ifdef DEBUGTEXT
      HIGAN_LOG("T%d: internalAddTaskWithoutRequirements name \"%s\" id: %d\n", std::this_thread::get_id(), name.c_str(), newId);
#endif
      Task newTask(newId, start_iter, iterations);
      // add to queue
      newTask.genWorkFunc<ppt>(std::forward<Func>(func));
      {
        ThreadData& worker = m_allThreads.at(ThreadID);

        std::lock(m_wfrMutex, *m_mutexes[ThreadID]);
        std::unique_lock<std::mutex> u1(m_wfrMutex, std::adopt_lock);
        std::unique_lock<std::mutex> u2(*m_mutexes[ThreadID], std::adopt_lock);

        worker.m_localDeque.push_front(std::move(newTask));
        worker.m_localQueueSize->store(worker.m_localDeque.size(), std::memory_order::memory_order_relaxed);
        TaskInfo asd(name, {});
        m_taskInfos.insert({ newId, std::move(asd) });
        u1.unlock();
        u2.unlock();
      }
      notifyAll();
    }

    template<size_t ppt, typename Func>
    inline void internalAddTask(int ThreadID, std::string name, Requirements pre, Requirements post, size_t start_iter, size_t iterations, Func&& func)
    {
      // need a temporary storage for tasks that haven't had requirements filled
      size_t newId = m_nextTaskID.fetch_add(1);
      assert(newId < m_nextTaskID);
#ifdef DEBUGTEXT
      HIGAN_LOG("T%d: internalAddTask name \"%s\" id: %d\n", std::this_thread::get_id(), name.c_str(), newId);
#endif
      Task newTask(newId, start_iter, iterations);
      newTask.genWorkFunc<ppt>(std::forward<Func>(func));
      {
        std::lock(m_wfrMutex, *m_mutexes[ThreadID]);
        std::unique_lock<std::mutex> u1(m_wfrMutex, std::adopt_lock);
        std::unique_lock<std::mutex> u2(*m_mutexes[ThreadID], std::adopt_lock);
        if (checkRequirements(pre))
        {
          //HIGAN_LOG("T%d: internalAddTask name %s, ready\n", std::this_thread::get_id(), name.c_str());

          TaskInfo asd(name, post);
          m_taskInfos.insert({ newId, std::move(asd) });
          ThreadData& worker = m_allThreads.at(ThreadID);
          worker.m_localDeque.push_front(std::move(newTask));
          worker.m_localQueueSize->store(worker.m_localDeque.size(), std::memory_order::memory_order_relaxed);
        }
        else  // wtf add to WAITING FOR REQUIREMENTS
        {
          //HIGAN_LOG("T%d: internalAddTask name %s, waiting\n", std::this_thread::get_id(), name.c_str());
          TaskInfo asd(name, post);
          m_taskInfos.insert({ newId, std::move(asd) });
          m_waitingPreRequirements.push_back({ std::move(pre), std::move(newTask) });
        }
        u1.unlock();
        u2.unlock();
      }
      notifyAll();
    }

    // generic checker, kind of done, NEEDS MUTEX GUARDED OUTSIDE
    // internal "Requirements" could have array of integers than strings, faster and simpler.
    // although doubtly any single task ever has that many requirements
    bool checkRequirements(Requirements& data)
    {
      // check and remove
      if (data.m_regs.size() == 0)
      {
        return true;
      }
      std::vector<std::string>::iterator newEnd;
      {
        newEnd = std::remove_if(data.m_regs.begin(), data.m_regs.end(), [&](const std::string& req)
        {
          auto it = m_fullfilled.find(req);
          if (it != m_fullfilled.end())
          {
            if (it->second)
            {
              it->second = false; // is this allowed !?
              return true;
            }
          }
          return false;
        });
      }
      data.m_regs.erase(newEnd, data.m_regs.end());
      return data.m_regs.size() == 0;
    }

    // Checks and does all postTask related work.
    // Includes adding new tasks that are waiting for the reported task.
    void postTaskWork(size_t taskID)
    {
#ifdef DEBUGTEXT
      HIGAN_LOG("T%d: postTaskWork id %llu\n", std::this_thread::get_id(), taskID);
#endif
      std::string taskname;
      {
        std::lock_guard<std::mutex> guard(m_wfrMutex);
        auto it = m_taskInfos.find(taskID);
        if (it == m_taskInfos.end()) return;

        auto& data = it->second;
        bool checkReq = false;
        checkReq = checkRequirements(data.m_post);
        if (checkReq)
        {
          // everything was finished!!!
          taskname = data.m_name;
          m_taskInfos.erase(it->first);
        }
        else
        {
          m_waitingPostRequirements.push_back({ std::move(data.m_post),std::move(data.m_name) });
          m_taskInfos.erase(it->first);
        }
      }
      if (taskname.length() == 0)
      {
        return;
      }
      informTaskFinished(taskname);
    }

    // Does bunch of stuff. Basically looks if there are tasks, and adds them.
    void informTaskFinished(std::string name)
    {
#ifdef DEBUGTEXT
      HIGAN_LOG("T%d: informTaskFinished name %s\n", std::this_thread::get_id(), name.c_str());
#endif
      std::vector<Task> addable;
      {
        std::lock_guard<std::mutex> guard(m_wfrMutex);
        m_fullfilled[name] = true;
        bool cont = true;
        while (cont)
        {
          cont = false;
          for (auto it = m_waitingPostRequirements.begin(); it != m_waitingPostRequirements.end(); it++)
          {
            if (checkRequirements(it->first))
            {
              m_fullfilled[it->second] = true;
              cont = true;
              m_waitingPostRequirements.erase(it);
              break;
            }
          }
        }
        bool cond = true;
        while (cond)
        {
          cond = false;
          for (auto it = m_waitingPreRequirements.begin(); it != m_waitingPreRequirements.end(); it++)
          {
            if (checkRequirements(it->first))
            {
              addable.push_back(std::move(it->second));
              m_waitingPreRequirements.erase(it);
              cond = true;
              break;
            }
          }
        }
      }
#ifdef DEBUGTEXT
      HIGAN_LOG("T%d: informTaskFinished name %s Exited reason success, added %llu tasks\n", std::this_thread::get_id(), name.c_str(), addable.size());
#endif
      {
        ThreadData& worker = m_allThreads.at(0);
        std::lock_guard<std::mutex> guard(*m_mutexes.at(0));
        for (auto& it : addable)
        {
          worker.m_localDeque.push_front(std::move(it));
          worker.m_localQueueSize->store(worker.m_localDeque.size(), std::memory_order::memory_order_relaxed);
          notifyAll();
        }
      }
    }

    // Main Worker loop, Meat of LBS algorithm is in here.
    void loop(int i)
    {
      ThreadData& p = m_allThreads.at(i);
      //p.setGlobalID();
      stealOrWait(p);
      while (!StopCondition)
      {
        // can we split work?
        if (p.m_task.canSplit())
        {
          if (p.m_localQueueSize->load() == 0)
          { // Queue didn't have anything, adding.
            {
              std::lock_guard<std::mutex> guard(*m_mutexes.at(p.m_ID));
              p.m_localDeque.push_back(p.m_task.split()); // push back
              p.m_localQueueSize->store(p.m_localDeque.size(), std::memory_order::memory_order_relaxed);
            }
            notifyAll();
            continue;
          }
        }
        // we couldn't split or queue had something
        doWork(p);
      }
    }

    // Mostly functions used by threads
    inline void doWork(ThreadData& p) {
      auto currentIterID = p.m_task.m_iterID;
      t_threadid = p.m_ID;
      t_reSchedule = false;
      // profiling name
      auto        it = m_taskInfos.find(p.m_task.m_id);
      const char* name = "";
      if (it != m_taskInfos.end()) {
        name = it->second.m_name.c_str();
      }
      // insert profiling begin
      bool rdy = p.m_task.doWork();
      // insert profiling, end event
      auto amountOfWork = p.m_task.m_iterID - currentIterID;
      if (t_reSchedule) p.m_task.m_sharedWorkCounter->operator++();
      didWorkFor(p.m_task, amountOfWork);
      if (rdy && t_reSchedule) {
        t_reSchedule = false;
        auto nTask = Task(p.m_task.m_id, p.m_task.m_originalIterID, p.m_task.m_originalIterations);
        nTask.f_work = p.m_task.f_work;
        {
          ThreadData&                 worker = m_allThreads.at(0);
          std::lock_guard<std::mutex> guard(*m_mutexes.at(0));
          worker.m_localDeque.push_front(std::move(nTask));
          worker.m_localQueueSize->store(worker.m_localDeque.size(), std::memory_order::memory_order_relaxed);
        }
        notifyAll();
      }
      if (rdy) {
        stealOrWait(p);
      }
    }

    // Goes through all the ways to get work and sleeps if nothing is found.
    inline void stealOrWait(ThreadData& p) {
      size_t id = p.m_task.m_id;
      while (id == p.m_task.m_id) {
        if (p.m_localQueueSize->load() != 0) {
          std::shared_ptr<std::mutex>& woot = m_mutexes.at(p.m_ID);
          std::lock_guard<std::mutex> guard(*woot);
          if (!p.m_localDeque.empty()) {
            p.m_task = std::move(p.m_localDeque.back());
            p.m_localDeque.pop_back();
            p.m_localQueueSize->store(p.m_localDeque.size(), std::memory_order::memory_order_relaxed);
            return;
          }
        }
        for (auto& it : m_allThreads) {
          if (it.m_ID == p.m_ID)
            continue;
          if (it.m_localQueueSize->load() != 0) {
            std::lock_guard<std::mutex> guard(*m_mutexes.at(it.m_ID));
            if (!it.m_localDeque.empty()) {
              p.m_task = it.m_localDeque.front();
              it.m_localDeque.pop_front();
              it.m_localQueueSize->store(it.m_localDeque.size(), std::memory_order::memory_order_relaxed);
              return;
            }
          }
        }
        if (!StopCondition) {
          if (p.m_localQueueSize->load() != 0) {
            continue;
          }
          std::unique_lock<std::mutex> lk(*m_mutexes[p.m_ID]);
          idle_threads++;
          m_cvs[p.m_ID]->wait(lk);
          idle_threads--;
        }
      }
    }

    // Reports the amount of task done and does post task work if was last.
    inline void didWorkFor(Task& task, size_t amount) {
      if (amount <= 0) {
        return;
      }
      if (task.m_sharedWorkCounter->fetch_sub(amount) - amount <= 0) {
        postTaskWork(task.m_id);
      }
    }
  };
};

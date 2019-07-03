#pragma once
#include <functional>
#include <string>
#include <vector>
#include <iostream>
#include "higanbana/core/system/logger.hpp"
#include "higanbana/core/global_debug.hpp"
#include "higanbana/core/tools/bentsumaakaa.hpp"

namespace higanbana
{
  class TestWorks
  {
  private:
    std::vector<std::pair<std::string, std::function<bool()> > >  m_tests;
    std::function<void()> m_before;
    std::function<void()> m_after;
    std::string m_name;
    //Logger logs;
    Bentsumaakaa b;
  public:
    TestWorks(std::string name)
      : m_before([]() {})
      , m_after([]() {})
      , m_name(name)
    {}
    void setBeforeTest(std::function<void()> func)
    {
      m_before = func;
    }

    void setAfterTest(std::function<void()> func)
    {
      m_after = func;
    }

    bool runTests()
    {
      F_LOG("----------------------------------------------------------------\n");
      F_LOG("Testing %s \n", m_name.c_str());
      int testCount = 0;
      F_LOG("Running %zu tests.\n", m_tests.size());
      F_LOG("----------------------------------------------------------------\n");
      for (auto& it : m_tests)
      {
        std::string ok = "[FAILED] ";
        m_before();
        b.start(false);
        bool a = it.second();
        auto time = b.stop(false);
        m_after();
        std::string padding = "";
        if (a)
        {
          ok = "[OK] ";
          ++testCount;
          padding = "    ";
        }
        auto timef = time / 1000000.f;
        if (timef < 10.f)
        {
          padding += " ";
        }
        if (timef < 100.f)
        {
          padding += " ";
        }
        F_LOG("%s %s%f ms Test %s\n", ok.c_str(),padding.c_str(), timef, it.first.c_str());
      };
      F_LOG("----------------------------------------------------------------\n");
      F_LOG("Tests %zu / %zu completed\n", testCount, m_tests.size());
      F_LOG("----------------------------------------------------------------\n");
      //logs.update();
      return (testCount - m_tests.size() == 0);
    }

    void addTest(std::string name, std::function<bool()> func)
    {
      m_tests.push_back({ name, func });
    }
  };

}

#pragma once
#include <functional>
#include <string>
#include <vector>
#include <iostream>
#include "core/src/system/logger.hpp"
#include "core/src/global_debug.hpp"

namespace faze
{
  class TestWorks
  {
  private:
    std::vector<std::pair<std::string, std::function<bool()> > >  m_tests;
    std::string m_name;
    Logger logs;
  public:
    TestWorks(std::string name) :m_name(name) {}

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
        if (it.second())
        {
          ok = "[OK] ";
          ++testCount;
        }
        F_LOG("%s Test %s\n", ok.c_str(), it.first.c_str());
      };
      F_LOG("----------------------------------------------------------------\n");
      F_LOG("Tests %zu / %zu completed\n", testCount, m_tests.size());
      F_LOG("----------------------------------------------------------------\n");
      logs.update();
      return (testCount - m_tests.size() == 0);
    }

    void addTest(std::string name, std::function<bool()> func)
    {
      m_tests.push_back({ name, func });
    }
  };

}

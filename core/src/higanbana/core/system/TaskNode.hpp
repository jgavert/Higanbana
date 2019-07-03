#pragma once
#include <vector>
#include <atomic>

namespace higanbana
{

  class TaskNode
  {
    size_t id;
    size_t type; // 0 = normal, 1 = mainthread... etc
    std::atomic<int> prereq; // used to detect if task can be done
    std::atomic<int> postreq; // used to detect if task can inform that its done
    std::vector<size_t> listeners; // contains all the nodes that need to be informed
    std::vector<size_t> prerequirementNodes; // all prerequirement nodes
    std::vector<size_t> postrequirementNodes; // all post...

  };
}

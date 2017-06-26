#pragma once
#include "core/src/system/memview.hpp"
#include "core/src/global_debug.hpp"
#include "core/src/datastructures/proxy.hpp"
#include <cstdio>
#include <vector>
#include <string>
#include <memory>

namespace faze
{

  class MemoryBlob
  {
  private:
    std::vector<uint8_t> m_data;
  public:
    MemoryBlob();
    MemoryBlob(std::vector<uint8_t> data);
    size_t size();
    uint8_t* data();
  };

  class FileSystem
  {
  private:
    faze::unordered_map<std::string, std::vector<uint8_t>> m_files;
    faze::unordered_set<std::string> m_dirs;
  public:
    FileSystem();
    bool fileExists(std::string path);
    MemoryBlob readFile(std::string path);
    faze::MemView<const uint8_t> viewToFile(std::string path);
    void loadDirectoryContentsRecursive(std::string path);
    bool loadFileFromHDD(std::string path, size_t& size);
    size_t timeModified(std::string path);

    bool writeFile(std::string path, const uint8_t* ptr, size_t size);
    bool writeFile(std::string path, faze::MemView<const uint8_t> view);

    /*
    file exists
    open file
    delayed write
    get filesnames in directory
    */
  };

}
#pragma once
#include "core/src/system/memview.hpp"
#include "core/src/global_debug.hpp"
#include "core/src/datastructures/proxy.hpp"
#include <cstdio>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <atomic>

namespace faze
{
  class WatchFile
  {
    std::shared_ptr<std::atomic<bool>> m_updated = nullptr;

  public:

    WatchFile()
    {}

    WatchFile(std::shared_ptr<std::atomic<bool>> updated)
      : m_updated(updated)
    {}

    void react()
    {
      if (m_updated)
        *m_updated = false;
    }

    bool updated()
    {
      if (m_updated)
        return *m_updated;
      return false;
    }

    void update()
    {
      *m_updated = true;
    }

    bool empty()
    {
      return !m_updated;
    }
  };

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

    struct FileObj
    {
      size_t timeModified;
      vector<uint8_t> data;
    };

    faze::unordered_map<std::string, FileObj> m_files;
    faze::unordered_set<std::string> m_dirs;

    // watch
    std::unordered_map<std::string, vector<WatchFile>> m_watchedFiles;
    int rollingUpdate = 0;

    // 
    std::mutex m_lock;

  public:
    FileSystem();
    bool fileExists(std::string path);
    MemoryBlob readFile(std::string path);
    faze::MemView<const uint8_t> viewToFile(std::string path);
    void loadDirectoryContentsRecursive(std::string path);
    size_t timeModified(std::string path);

    bool writeFile(std::string path, const uint8_t* ptr, size_t size);
    bool writeFile(std::string path, faze::MemView<const uint8_t> view);

    WatchFile watchFile(std::string path);
    void updateWatchedFiles();
  private:
    bool loadFileFromHDD(std::string path, size_t& size);
  };

}
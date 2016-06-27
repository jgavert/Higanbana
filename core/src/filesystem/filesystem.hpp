#pragma once
#include <fstream>
#include <iostream>
#include <ostream>
#include <cstring>
#include <deque>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>


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
  std::unordered_map<std::string, std::vector<uint8_t>> m_files;
  std::unordered_set<std::string> m_dirs;
  std::deque<std::string> m_writtenFiles;
public:
  FileSystem(std::string baseDir);
  bool fileExists(std::string path);
  MemoryBlob readFile(std::string path);

  template <typename T>
  bool writeFile(std::string path, T* ptr, size_t size)
  {
    std::vector<uint8_t> data(size, 0);
    memcpy(data.data(), reinterpret_cast<const uint8_t*>(ptr), size);
    if (fileExists(path))
    {
      // replace
      m_files[path] = std::move(data);
      m_writtenFiles.push_back(path);
      return true;
    }
    // check if there is a dir
    // createNewFile
    return false;
  }

  void flushFiles();
};

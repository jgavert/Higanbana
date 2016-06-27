#include "filesystem.hpp"
#include "../global_debug.hpp" 
#include <dirent.h>

void getDirs(std::string path, std::deque<std::string>& ret)
{
  DIR* dir;
  struct dirent* ent;

  if ((dir = opendir (path.c_str())) != NULL)
  {
    /* print all the files and directories within directory */
    while ((ent = readdir (dir)) != NULL)
    {
      if (ent->d_type & DT_DIR)
      {
        auto name = std::string(ent->d_name);
        if (name != ".." && name != ".")
        {
          ret.push_back(path + "/" + name);
        }
      }
    }
    closedir (dir);
  } else {
    /* could not open directory */
    perror ("");
  }
}

void getFiles(std::string path, std::vector<std::string>& ret)
{
  DIR* dir;
  struct dirent* ent;

  if ((dir = opendir (path.c_str())) != NULL)
  {
    /* print all the files and directories within directory */
    while ((ent = readdir (dir)) != NULL)
    {
      if (!(ent->d_type & DT_DIR))
      {
        auto name = std::string(ent->d_name);
        if (name != ".." && name != ".")
        {
          ret.emplace_back(path + "/" + name);
        }
      }
    }
    closedir (dir);
  } else {
    /* could not open directory */
    perror ("");
  }
}


MemoryBlob::MemoryBlob()
{
}

MemoryBlob::MemoryBlob(std::vector<uint8_t> data)
  : m_data(data)
{
}

size_t MemoryBlob::size()
{
  return m_data.size();
}

uint8_t* MemoryBlob::data()
{
  return m_data.data();
}

FileSystem::FileSystem(std::string baseDir)
{
  std::deque<std::string> dirs;
  std::vector<std::string> files;
  getDirs(baseDir.c_str(), dirs);
  getFiles(baseDir.c_str(), files);
  while(!dirs.empty())
  {
    auto getOne = dirs.front();
    dirs.pop_front();
    m_dirs.insert(getOne);
    F_SLOG("FileSystem", "found dir %s\n", getOne.c_str());
    getDirs(getOne, dirs);
    getFiles(getOne, files);
  }
  size_t allSize = 0;
  for (auto&& it : files)
  {
    auto gn = it.substr(1,it.size());
    std::ifstream filedata(it.c_str(), std::ios::binary);
    if (filedata.good())
    {
      size_t size = 0;
      filedata.seekg(0, std::ios::end);
      size = static_cast<size_t>(filedata.tellg());
      allSize += size;
      F_SLOG("FileSystem", "found file %s, loading %.2fMB(%zu)...\n", it.c_str(), static_cast<float>(size)/1000000.f, size);
      filedata.seekg(0, std::ios::beg);
      std::vector<uint8_t> contents(size, '\0');
      filedata.read(reinterpret_cast<char*>(contents.data()), size);
      m_files[gn] = std::move(contents);
    }
    filedata.close();
  }
  F_SLOG("FileSystem", "found and loaded %zu files(%.2fMB total)\n", files.size(), static_cast<float>(allSize) / 1000000.f);
  // load all files to memory
}

bool FileSystem::fileExists(std::string path)
{
  return (m_files.find(path) != m_files.end());
}

MemoryBlob FileSystem::readFile(std::string path)
{
  MemoryBlob blob;
  if (fileExists(path))
  {
    blob = MemoryBlob(m_files[path]);
  }
  return blob;
}

void FileSystem::flushFiles()
{
  for (auto&& it : m_writtenFiles)
  {
    auto correctPath = "." + it;
    std::ofstream filedata(correctPath.c_str(), std::ios::binary);
    if (filedata.good())
    {
      F_SLOG("FileSystem","writing out file %s\n", correctPath.c_str());
      auto& fdata = m_files[it];
      filedata.write(reinterpret_cast<char*>(fdata.data()), fdata.size());
    }
    filedata.close();
  }
}

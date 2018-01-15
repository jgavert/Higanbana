#include "filesystem.hpp"
#include "../global_debug.hpp"

#include <deque>

#include <experimental/filesystem>

#define FAZE_FS_EXTRA_INFO 1

#if FAZE_FS_EXTRA_INFO
#define FS_ILOG(msg, ...) F_ILOG("Filesystem", msg, ##__VA_ARGS__)
#define FS_LOG(msg, ...) F_SLOG("Filesystem", msg, ##__VA_ARGS__)
#else
#define FS_ILOG(msg, ...)
#define FS_LOG(msg, ...)
#endif

namespace system_fs = std::experimental::filesystem;
using namespace faze;

void getDirs(std::string path, std::deque<std::string>& ret)
{
  if (system_fs::is_directory(path))
  {
    auto folder = system_fs::directory_iterator(path);
    for (auto& file : folder)
    {
      if (system_fs::is_directory(file.path()))
      {
        ret.push_back(file.path().string());
      }
    }
  }
}

void getDirsRecursive(std::string path, std::deque<std::string>& ret)
{
  if (system_fs::is_directory(path))
  {
    auto folder = system_fs::recursive_directory_iterator(path);
    for (auto& file : folder)
    {
      if (system_fs::is_directory(file.path()))
      {
        ret.push_back(file.path().string());
      }
    }
  }
}

void getFiles(std::string path, std::vector<std::string>& ret)
{
  if (system_fs::is_directory(path))
  {
    auto folder = system_fs::directory_iterator(path);
    for (auto& file : folder)
    {
      if (system_fs::is_regular_file(file.path()))
      {
        ret.push_back(file.path().string());
      }
    }
  }
}

void getFilesRecursive(std::string path, std::vector<std::string>& ret)
{
  if (system_fs::is_directory(path))
  {
    auto folder = system_fs::recursive_directory_iterator(path);
    for (auto& file : folder)
    {
      if (system_fs::is_regular_file(file.path()))
      {
        ret.push_back(file.path().string());
      }
    }
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

FileSystem::FileSystem()
{
  loadDirectoryContentsRecursive("");
}

bool FileSystem::fileExists(std::string path)
{
  std::lock_guard<std::mutex> guard(m_lock);
  auto fullPath = system_fs::path(system_fs::current_path().string() + path).string();
#if defined(FAZE_PLATFORM_WINDOWS)
  std::replace(fullPath.begin(), fullPath.end(), '/', '\\');
#endif
  return (m_files.find(fullPath) != m_files.end());
}

bool FileSystem::loadFileFromHDD(std::string path, size_t& size)
{
  // convert all \ to /
#if defined(FAZE_PLATFORM_WINDOWS)
  std::replace(path.begin(), path.end(), '/', '\\');
#endif
  auto fp = fopen(path.c_str(), "rb");
  fseek(fp, 0L, SEEK_END);
  auto fsize = ftell(fp);
  fseek(fp, 0L, SEEK_SET);
  std::vector<uint8_t> contents(fsize);
  size_t leftToRead = fsize;
  size_t offset = 0;
  while (leftToRead > 0)
  {
    offset = fread(contents.data() + offset, sizeof(uint8_t), leftToRead, fp);
    if (offset == 0)
      break;
    leftToRead -= offset;
  }
  fclose(fp);
  //contents.resize(size - leftToRead);
  auto time = static_cast<size_t>(system_fs::last_write_time(path).time_since_epoch().count());
  FS_ILOG("found file %s(%zu), loading %.2fMB(%ld)...", path.c_str(), time, static_cast<float>(fsize) / 1024.f / 1024.f, fsize);
  m_files[path] = FileObj{ time, contents };
  size = fsize;
  return true;
}

void FileSystem::loadDirectoryContentsRecursive(std::string path)
{
  std::lock_guard<std::mutex> guard(m_lock);
  auto fullPath = system_fs::current_path().string() + path;
  std::vector<std::string> files;
  getFilesRecursive(fullPath.c_str(), files);
  size_t allSize = 0;
  for (auto&& it : files)
  {
    size_t fileSize = 0;
    loadFileFromHDD(it, fileSize);
    allSize += fileSize;
  }
  FS_ILOG("found and loaded %zu files(%.2fMB total)", files.size(), static_cast<float>(allSize) / 1024.f / 1024.f);
}

// SLOW
void FileSystem::getFilesWithinDir(std::string path, std::function<void(std::string&, MemView<const uint8_t>)> func)
{
  std::lock_guard<std::mutex> guard(m_lock);
  const auto currentPath = system_fs::current_path().string();
  auto fullPath = currentPath + path;
  std::vector<std::string> files;
  getFilesRecursive(fullPath.c_str(), files);
  for (auto&& it : files)
  {
    auto f = m_files.find(it);
    if (f != m_files.end())
    {
      auto localPath = it.substr(fullPath.size());
      func(localPath, faze::MemView<const uint8_t>(f->second.data));
    }
  }
}

MemoryBlob FileSystem::readFile(std::string path)
{
  MemoryBlob blob;
  if (fileExists(path))
  {
    std::lock_guard<std::mutex> guard(m_lock);
    auto fullPath = system_fs::path(system_fs::current_path().string() + path).string();
    auto& file = m_files[fullPath];
    blob = MemoryBlob(file.data);
  }
  return blob;
}

faze::MemView<const uint8_t> FileSystem::viewToFile(std::string path)
{
  faze::MemView<const uint8_t> view;
  if (fileExists(path))
  {
    std::lock_guard<std::mutex> guard(m_lock);
    auto fullPath = system_fs::path(system_fs::current_path().string() + path).string();
    view = faze::MemView<const uint8_t>(m_files[fullPath].data);
  }
  return view;
}

size_t FileSystem::timeModified(std::string path)
{
  auto fullPath = system_fs::path(system_fs::current_path().string() + path).string();
  return system_fs::last_write_time(fullPath).time_since_epoch().count();
}

bool FileSystem::writeFile(std::string path, faze::MemView<const uint8_t> view)
{
  return writeFile(path, view.data(), view.size());
}

bool FileSystem::writeFile(std::string path, const uint8_t* ptr, size_t size)
{
  std::lock_guard<std::mutex> guard(m_lock);
  auto fullPath = system_fs::path(system_fs::current_path().string() + path);

  std::vector<uint8_t> fdata(size);
  memcpy(fdata.data(), reinterpret_cast<const uint8_t*>(ptr), size);
  // replace
  // check if there is a dir
  // createNewFile

  // old filewrite
  if (!system_fs::exists(fullPath.parent_path()))
  {
    if (!system_fs::create_directories(fullPath.parent_path()))
    {
      return false;
    }
  }
  else if (system_fs::exists(fullPath))
  {
    F_ASSERT(system_fs::is_regular_file(fullPath), "Tried to overwrite somethign that wasn't a regular file.");
  }
  auto freeSpace = system_fs::space(fullPath.parent_path());
  if (freeSpace.available < size)
  {
    F_ASSERT(false, "no disk space !?!?!?!?");
    return false;
  }
  auto file = fopen(fullPath.string().c_str(), "wb");
  FS_LOG("writing out file %s\n", fullPath.string().c_str());
  size_t leftToWrite = size;
  size_t offset = 0;
  while (leftToWrite > 0)
  {
    offset = fwrite(ptr + offset, sizeof(uint8_t), leftToWrite, file);
    leftToWrite -= offset;
  }

  fclose(file);
  system_fs::resize_file(fullPath, size);
  m_files[fullPath.string()].data = std::move(fdata);
  return true;
}

WatchFile FileSystem::watchFile(std::string path)
{
  if (fileExists(path))
  {
    std::lock_guard<std::mutex> guard(m_lock);
    WatchFile w = WatchFile(std::make_shared<std::atomic<bool>>());
    auto fullPath = system_fs::current_path().string() + path;
    std::replace(fullPath.begin(), fullPath.end(), '/', '\\');
    m_watchedFiles[fullPath].push_back(w);
    return w;
  }
  return WatchFile{};
}

void FileSystem::updateWatchedFiles()
{
  std::lock_guard<std::mutex> guard(m_lock);
  if (m_watchedFiles.empty())
  {
    return;
  }
  if (rollingUpdate >= static_cast<int>(m_watchedFiles.size()))
  {
    rollingUpdate = 0;
  }
  auto current = m_watchedFiles.begin();
  for (int i = 0; i < rollingUpdate; ++i)
  {
    current++;
  }
  auto oldTime = m_files[current->first].timeModified;
  auto newTime = static_cast<size_t>(system_fs::last_write_time(current->first).time_since_epoch().count());
  if (newTime > oldTime)
  {
    size_t opt;
    loadFileFromHDD(current->first, opt);
    for (auto&& it : current->second)
    {
      it.update();
    }
  }
  rollingUpdate++;
}

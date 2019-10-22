#include "higanbana/core/filesystem/filesystem.hpp"
#include "higanbana/core/global_debug.hpp"

#include <deque>

#include <filesystem>

#define HIGANBANA_FS_EXTRA_INFO 0

#if HIGANBANA_FS_EXTRA_INFO
#define FS_ILOG(msg, ...) HIGAN_ILOG("Filesystem", msg, ##__VA_ARGS__)
#define FS_LOG(msg, ...) HIGAN_SLOG("Filesystem", msg, ##__VA_ARGS__)
#else
#define FS_ILOG(msg, ...)
#define FS_LOG(msg, ...)
#endif

namespace system_fs = std::filesystem;
using namespace higanbana;

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


void getFiles(std::string path, std::vector<FileInfo>& ret)
{
  if (system_fs::is_directory(path))
  {
    auto pathSize = path.size();
    auto folder = system_fs::directory_iterator(path);
    for (auto& file : folder)
    {
      if (system_fs::is_regular_file(file.path()))
      {
        auto withoutNative = file.path().string().substr(pathSize);
        std::replace(withoutNative.begin(), withoutNative.end(), '\\', '/');
        ret.push_back({file.path().string(), withoutNative});
      }
    }
  }
}

void getFilesRecursive(std::string path, size_t pathSize, std::vector<FileInfo>& ret)
{
  if (system_fs::is_directory(path))
  {
    auto folder = system_fs::recursive_directory_iterator(path);
    for (auto& file : folder)
    {
      if (system_fs::is_regular_file(file.path()))
      {
        auto withoutNative = file.path().string().substr(pathSize);
        std::replace(withoutNative.begin(), withoutNative.end(), '\\', '/');
        ret.push_back({file.path().string(), withoutNative});
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

size_t MemoryBlob::size() const noexcept
{
  return m_data.size();
}

uint8_t* MemoryBlob::data() noexcept
{
  return m_data.data();
}

const uint8_t* MemoryBlob::cdata() const noexcept
{
  return m_data.data();
}

FileSystem::FileSystem()
  : m_resolvedFullPath(system_fs::path(system_fs::current_path().string()).string())
{
  loadDirectoryContentsRecursive("");
  auto fullPath = system_fs::current_path().string();
  HIGAN_ILOG("FileSystem", "Working directory: \"%s\"", fullPath.c_str());
}

FileSystem::FileSystem(std::string relativeOffset)
  : m_resolvedFullPath(system_fs::path(system_fs::current_path().string() + relativeOffset).string())
{
  loadDirectoryContentsRecursive("");
  auto fullPath = getBasePath();
  HIGAN_ILOG("FileSystem", "Working directory: \"%s\"", fullPath.c_str());
}

std::string FileSystem::getBasePath()
{
  return m_resolvedFullPath;
}

bool FileSystem::fileExists(std::string path)
{
  std::lock_guard<std::mutex> guard(m_lock);
  //auto fullPath = system_fs::path(getBasePath() + path).string();
#if defined(HIGANBANA_PLATFORM_WINDOWS)
  //std::replace(fullPath.begin(), fullPath.end(), '/', '\\');
#endif
  auto found = m_files.find(path);
  auto end = m_files.end();
  auto hasFile = found != end;
  FS_ILOG("checking %s... %s\n", path.c_str(), hasFile ? "found" : "error");
  return hasFile;
}

bool FileSystem::loadFileFromHDD(FileInfo& path, size_t& size)
{
  // convert all \ to /
#if defined(HIGANBANA_PLATFORM_WINDOWS)
  std::replace(path.nativePath.begin(), path.nativePath.end(), '/', '\\');
#endif
  auto fp = fopen(path.nativePath.c_str(), "rb");
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
  auto time = static_cast<size_t>(system_fs::last_write_time(path.nativePath).time_since_epoch().count());
  FS_ILOG("found file %s(%zu), loading %.2fMB(%ld)...", path.nativePath.c_str(), time, static_cast<float>(fsize) / 1024.f / 1024.f, fsize);
  m_files[path.withoutNative] = FileObj{ time, contents };
  size = fsize;
  return true;
}

void FileSystem::loadDirectoryContentsRecursive(std::string path)
{
  std::lock_guard<std::mutex> guard(m_lock);
  auto fullPath = getBasePath() + path;
  std::vector<FileInfo> files;
  getFilesRecursive(fullPath.c_str(), getBasePath().size(), files);
  size_t allSize = 0;
  for (auto&& it : files)
  {
    size_t fileSize = 0;
    FS_ILOG("wanting to load %s, converting path to \"%s\"", it.nativePath.c_str(), it.withoutNative.c_str());
    loadFileFromHDD(it, fileSize);
    allSize += fileSize;
  }
  FS_ILOG("found and loaded %zu files(%.2fMB total)", files.size(), static_cast<float>(allSize) / 1024.f / 1024.f);
}

// SLOW
void FileSystem::getFilesWithinDir(std::string path, std::function<void(std::string&, MemView<const uint8_t>)> func)
{
  std::lock_guard<std::mutex> guard(m_lock);
  const auto currentPath = getBasePath();
  auto fullPath = currentPath + path;
  std::vector<FileInfo> files;
  getFilesRecursive(fullPath.c_str(), currentPath.size(), files);
  for (auto&& it : files)
  {
    auto f = m_files.find(it.withoutNative);
    if (f != m_files.end())
    {
      func(it.withoutNative, higanbana::MemView<const uint8_t>(f->second.data));
    }
  }
}

vector<std::string> FileSystem::getFilesWithinDir(std::string path)
{
  std::lock_guard<std::mutex> guard(m_lock);
  const auto currentPath = getBasePath();
  auto fullPath = currentPath + path;
  std::vector<FileInfo> natfiles;
  getFiles(fullPath.c_str(), natfiles);
  std::vector<std::string> files;
  std::for_each(natfiles.begin(), natfiles.end(), [&](FileInfo& file){
    files.push_back(file.withoutNative.substr(path.size()));
  });
  return files;
}


vector<std::string> FileSystem::recursiveList(std::string path, std::string filter)
{
  std::lock_guard<std::mutex> guard(m_lock);
  const auto currentPath = getBasePath();
  auto fullPath = currentPath + path;
  std::vector<FileInfo> files;
  getFilesRecursive(fullPath.c_str(), currentPath.size(), files);
  std::vector<std::string> outFiles;
  std::for_each(files.begin(), files.end(), [&](FileInfo& file){
    auto outPath = file.withoutNative.substr(path.size());
    if (outPath.find(filter) != std::string::npos)
      outFiles.emplace_back(outPath);
  });
  return outFiles;
}

MemoryBlob FileSystem::readFile(std::string path)
{
  MemoryBlob blob;
  if (fileExists(path))
  {
    FS_ILOG("Reading... %s\n", path.c_str());
    std::lock_guard<std::mutex> guard(m_lock);
    auto& file = m_files[path];
    blob = MemoryBlob(file.data);
  }
  return blob;
}

higanbana::MemView<const uint8_t> FileSystem::viewToFile(std::string path)
{
  higanbana::MemView<const uint8_t> view;
  if (fileExists(path))
  {
    std::lock_guard<std::mutex> guard(m_lock);
    view = higanbana::MemView<const uint8_t>(m_files[path].data);
  }
  return view;
}

size_t FileSystem::timeModified(std::string path)
{
  auto fullPath = system_fs::path(getBasePath() + path).string();
  return system_fs::last_write_time(fullPath).time_since_epoch().count();
}

bool FileSystem::writeFile(std::string path, higanbana::MemView<const uint8_t> view)
{
  return writeFile(path, view.data(), view.size());
}

bool FileSystem::writeFile(std::string path, const uint8_t* ptr, size_t size)
{
  std::lock_guard<std::mutex> guard(m_lock);
  auto fullPath = system_fs::path(getBasePath() + path);

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
    HIGAN_ASSERT(system_fs::is_regular_file(fullPath), "Tried to overwrite somethign that wasn't a regular file.");
  }
  auto freeSpace = system_fs::space(fullPath.parent_path());
  if (freeSpace.available < size)
  {
    HIGAN_ASSERT(false, "no disk space !?!?!?!?");
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
  m_files[path].data = std::move(fdata);
  return true;
}

WatchFile FileSystem::watchFile(std::string path)
{
  if (fileExists(path))
  {
    std::lock_guard<std::mutex> guard(m_lock);
    WatchFile w = WatchFile(std::make_shared<std::atomic<bool>>());
    m_watchedFiles[path].push_back(w);
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
  auto fullPath = getBasePath() + current->first;
  std::replace(fullPath.begin(), fullPath.end(), '/', '\\');
  auto newTime = static_cast<size_t>(system_fs::last_write_time(fullPath).time_since_epoch().count());
  if (newTime > oldTime)
  {
    size_t opt;
    FileInfo info = {fullPath, current->first};
    loadFileFromHDD(info, opt);
    for (auto&& it : current->second)
    {
      it.update();
    }
  }
  rollingUpdate++;
}
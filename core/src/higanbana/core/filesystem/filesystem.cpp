#include "higanbana/core/filesystem/filesystem.hpp"
#include "higanbana/core/profiling/profiling.hpp"
#include "higanbana/core/system/time.hpp"
#include "higanbana/core/global_debug.hpp"

#include <nlohmann/json.hpp>
#include <deque>
#include <filesystem>
#include <iostream>

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
  HIGAN_CPU_FUNCTION_SCOPE();
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
  HIGAN_CPU_FUNCTION_SCOPE();
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
  HIGAN_CPU_FUNCTION_SCOPE();
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

void getFilesRecursive(std::string path, std::string basePath, std::vector<FileInfo>& ret)
{
  HIGAN_CPU_FUNCTION_SCOPE();
  size_t pathSize = basePath.size();
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

std::vector<uint8_t> readFileNative(const char* path) {
  auto fp = fopen(path, "rb");
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
  return contents;
}

std::optional<std::unordered_map<std::string, std::string>> FileSystem::tryExtractMappings(std::string mappingjsonpath) {
  std::optional<std::unordered_map<std::string, std::string>> ret;
  if (system_fs::exists(mappingjsonpath)) {
    try {
      auto data = readFileNative(mappingjsonpath.c_str());
      std::string str = std::string(reinterpret_cast<const char*>(data.data()));
      auto fail = nlohmann::json::parse(data.data(), data.data()+data.size());
      std::unordered_map<std::string, std::string> mappings;
      auto mapping = fail["mappings"];
      for (auto& [key, value] : mapping.items()) {
        mappings[key] = value.get<std::string>();
      }
      ret = mappings;
    }
    catch(const nlohmann::json::parse_error& error){
      std::cout << error.what() << '\n';
    }
  }
  return ret;
} 

std::string FileSystem::mountPoint(std::string_view filepath) {
  auto getFirst = filepath.find('/', 1);
  return std::string(filepath.substr(0, getFirst));
}
std::string FileSystem::mountPointOSPath(std::string_view filepath) {
  auto mp = mountPoint(filepath);
  HIGAN_ASSERT(m_mappings.find(mp) != m_mappings.end(), "uups");
  return m_mappings[mp];
}

std::optional<std::string> FileSystem::resolveNativePath(std::string_view filepath) {
  auto getFirst = filepath.find('/', 1);
  auto mountPoint = std::string(filepath.substr(0, getFirst));
  auto f = m_mappings.find(mountPoint);
  if (f != m_mappings.end()) {
    auto name = f->second;
    if (getFirst < filepath.size())
      name += std::string(filepath.substr(getFirst));
    std::replace(name.begin(), name.end(), '/', '\\');
    //HIGAN_LOGi("base path: \"%s\" => \"%s\"\n", filepath.data(), name.c_str());
    return name;
  }
  return std::optional<std::string>();
}

FileSystem::FileSystem()
{
}

FileSystem::FileSystem(std::string relativeOffset, MappingMode mode, const char* mappingFileName)
{
  auto ourPath = system_fs::current_path().string();
  auto mappings = tryExtractMappings(ourPath + relativeOffset + "/mapping.json");
  //HIGAN_LOGi("working dir \"%s\" => \"%s\"\n", ourPath.c_str(), mappingFileName);
  if (!mappings)
    mappings = tryExtractMappings(ourPath + "/" + mappingFileName);
  if (mappings) {
    auto items = mappings.value();
    for (auto& pair : items) {
      HIGAN_LOGi("found mapping, \"%s\" => \"%s\"\n", pair.first.c_str(), pair.second.c_str());
    }
    m_mappings = items;
  }
  else {
    HIGAN_ASSERT(false, "mapping file not found, error error!");
  }
}

void FileSystem::initialLoad() {
  for (auto& [key, val] : m_mappings) {
    loadDirectoryContentsRecursive(key);
  }
  m_initialLoadComplete = true;
}

std::string FileSystem::directoryPath(std::string filePath) {
  return system_fs::path(filePath).parent_path().string();
}

bool FileSystem::tryLoadFile(std::string path) {
  HIGAN_CPU_FUNCTION_SCOPE();
  std::lock_guard<std::mutex> guard(m_lock);
  auto fullPath = resolveNativePath(path).value();
  FileInfo info;
  info.nativePath = fullPath;
  auto mp = mountPoint(path);
  info.withoutNative = path.substr(mp.size());
  size_t lol = 0;
  return loadFileFromHDD(info, mp, lol);
}

bool FileSystem::fileExists(std::string path)
{
  HIGAN_CPU_FUNCTION_SCOPE();
  if (!m_initialLoadComplete)
    initialLoad();
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

bool FileSystem::loadFileFromHDD(FileInfo& path, std::string mountpoint, size_t& size)
{
  HIGAN_CPU_FUNCTION_SCOPE();
  // convert all \ to /
#if defined(HIGANBANA_PLATFORM_WINDOWS)
  std::replace(path.nativePath.begin(), path.nativePath.end(), '/', '\\');
#endif
  auto last5 = path.withoutNative.substr(path.withoutNative.size()-5);
  if (strcmp("trace", last5.c_str()) == 0)
    return false;
  std::vector<uint8_t> contents = readFileNative(path.nativePath.c_str());
  //contents.resize(size - leftToRead);
  auto time = static_cast<size_t>(system_fs::last_write_time(path.nativePath).time_since_epoch().count());
  auto fsize = contents.size();
  FS_ILOG("found file %s(%zu), loading %.2fMB(%ld)...", path.nativePath.c_str(), time, static_cast<float>(fsize) / 1024.f / 1024.f, fsize);
  auto fullpath = mountpoint + path.withoutNative;
  m_files[fullpath] = FileObj{ time, contents };
  size = contents.size();
  return true;
}

void FileSystem::loadDirectoryContentsRecursive(std::string path)
{
  HIGAN_CPU_FUNCTION_SCOPE();
  std::lock_guard<std::mutex> guard(m_lock);
  auto targetmount = mountPointOSPath(path);
  auto mp = mountPoint(path);
  auto fullPath = resolveNativePath(path).value();
  std::vector<FileInfo> files;
  getFilesRecursive(fullPath, targetmount, files);
  size_t allSize = 0;
  Timer loadSpeed;
  for (auto&& it : files)
  {
    size_t fileSize = 0;
    auto newPath = mp + it.withoutNative;
    FS_ILOG("wanting to load %s, converting path to \"%s\"", it.nativePath.c_str(), newPath.c_str());
    loadFileFromHDD(it, mp, fileSize);
    allSize += fileSize;
  }
  int64_t micros = loadSpeed.timeMicro();
  if (!files.empty()) {
    double bandwidth = (double(allSize)/ 1000.0 / 1000.0 / 1000.0) / (double(micros)/1000.0/1000.0);
    HIGAN_ILOG("Filesystem", "found and loaded %zu files(%.2fMB total, %.3fGB/s)", files.size(), static_cast<float>(allSize) / 1024.f / 1024.f, bandwidth);
  }
}

// SLOW
void FileSystem::getFilesWithinDir(std::string path, std::function<void(std::string&, MemView<const uint8_t>)> func)
{
  HIGAN_CPU_FUNCTION_SCOPE();
  std::lock_guard<std::mutex> guard(m_lock);
  const auto ospath = mountPointOSPath(path);
  const auto mp = mountPoint(path);
  auto fullPath = resolveNativePath(path);
  std::vector<FileInfo> files;
  getFilesRecursive(fullPath.value(), ospath, files);
  for (auto&& it : files)
  {
    auto f = m_files.find(mp + it.withoutNative);
    if (f != m_files.end())
    {
      func(it.withoutNative, higanbana::MemView<const uint8_t>(f->second.data));
    }
  }
}

vector<std::string> FileSystem::getFilesWithinDir(std::string path)
{
  HIGAN_CPU_FUNCTION_SCOPE();
  std::lock_guard<std::mutex> guard(m_lock);
  const auto mountpoint = mountPoint(path);
  auto fullPath = mountpoint + path;
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
  HIGAN_CPU_FUNCTION_SCOPE();
  std::lock_guard<std::mutex> guard(m_lock);
  const auto mp = mountPoint(path);
  const auto osPath = mountPointOSPath(path);
  auto fullPath = resolveNativePath(path).value();
  std::vector<FileInfo> files;
  getFilesRecursive(fullPath, osPath, files);
  std::vector<std::string> outFiles;
  std::for_each(files.begin(), files.end(), [&](FileInfo& file){
    auto outPath = mp + file.withoutNative;
    if (outPath.find(filter) != std::string::npos)
      outFiles.emplace_back(outPath);
  });
  return outFiles;
}

MemoryBlob FileSystem::readFile(std::string path)
{
  HIGAN_CPU_FUNCTION_SCOPE();
  MemoryBlob blob;
  if (!m_initialLoadComplete)
    initialLoad();
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
  HIGAN_CPU_FUNCTION_SCOPE();
  if (!m_initialLoadComplete)
    initialLoad();
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
  auto fullPath = system_fs::path(resolveNativePath(path).value()).string();
  return system_fs::last_write_time(fullPath).time_since_epoch().count();
}

bool FileSystem::writeFile(std::string path, higanbana::MemView<const uint8_t> view)
{
  HIGAN_CPU_FUNCTION_SCOPE();
  return writeFile(path, view.data(), view.size());
}

bool FileSystem::writeFile(std::string path, const uint8_t* ptr, size_t size)
{
  HIGAN_CPU_FUNCTION_SCOPE();
  std::lock_guard<std::mutex> guard(m_lock);
  auto fullPath = system_fs::path(resolveNativePath(path).value());

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
      FS_LOG("failed to write file %s\n", fullPath.string().c_str());
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

void FileSystem::addWatchDependency(std::string watchedPath, std::string notifyPath) {
  HIGAN_CPU_FUNCTION_SCOPE();
  if (fileExists(watchedPath))
  {
    std::lock_guard<std::mutex> guard(m_lock);
    auto dependency = m_dependencies.find(watchedPath);
    if (dependency != m_dependencies.end()) {
      bool hasAlready = false;
      for (auto&& existingPath : dependency->second)
        if (existingPath.compare(notifyPath) == 0) {
          hasAlready = true;
          break;
        }
      if (!hasAlready)
        dependency->second.push_back(notifyPath);
    }
    else {
      m_dependencies[watchedPath].push_back(notifyPath);
    }
  }
}

WatchFile FileSystem::watchFile(std::string path, std::optional<vector<std::string>> optionalTriggers) {
  HIGAN_CPU_FUNCTION_SCOPE();
  if (optionalTriggers) {
    for (auto& addPath : optionalTriggers.value()) {
      addWatchDependency(addPath, path);
    }
  }
  if (fileExists(path))
  {
    std::lock_guard<std::mutex> guard(m_lock);
    auto foundWatch = m_watchedFiles.find(path);
    if (foundWatch != m_watchedFiles.end())
      return foundWatch->second;

    WatchFile w = WatchFile(std::make_shared<std::atomic<bool>>());
    m_watchedFiles[path] = w;
    m_dependencies[path];
    return w;
  }
  return WatchFile{};
}

void FileSystem::updateWatchedFiles()
{
  HIGAN_CPU_FUNCTION_SCOPE();
  std::lock_guard<std::mutex> guard(m_lock);
  if (m_dependencies.empty())
  {
    return;
  }
  if (rollingUpdate >= static_cast<int>(m_dependencies.size()))
  {
    rollingUpdate = 0;
  }
  auto current = m_dependencies.begin();
  for (int i = 0; i < rollingUpdate; ++i)
  {
    current++;
  }
  auto oldTime = m_files[current->first].timeModified;
  auto fullPath = resolveNativePath(current->first).value();
  auto mp = mountPoint(current->first);
  std::replace(fullPath.begin(), fullPath.end(), '/', '\\');
  auto newTime = static_cast<size_t>(system_fs::last_write_time(fullPath).time_since_epoch().count());
  if (newTime > oldTime)
  {
    size_t opt;
    FileInfo info = {fullPath, current->first.substr(mp.size())};
    loadFileFromHDD(info, mp, opt);
    auto origFile = m_watchedFiles.find(current->first);
    if (origFile != m_watchedFiles.end()) {
      origFile->second.update();
    }
    for (auto& informPaths : current->second){
      auto watchFile = m_watchedFiles.find(informPaths);
      if (watchFile != m_watchedFiles.end()) {
        watchFile->second.update();
      }
    }
  }
  rollingUpdate++;
}
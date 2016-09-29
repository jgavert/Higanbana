#include "filesystem.hpp"
#include "../global_debug.hpp"

#include <deque>

#include <experimental/filesystem>

namespace system_fs = std::experimental::filesystem;

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
  auto fullPath = system_fs::path(system_fs::current_path().string() + path).string();
  return (m_files.find(fullPath) != m_files.end());
}

bool FileSystem::loadFileFromHDD(std::string path, size_t& size)
{
	auto fp = fopen(path.c_str(), "rb");
	fseek(fp, 0L, SEEK_END);
	auto fsize = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	F_ILOG("FileSystem", "found file %s, loading %.2fMB(%ld)...", path.c_str(), static_cast<float>(size) / 1000000.f, fsize);
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
	m_files[path] = std::move(contents);
	size = fsize;
	return true;
}

void FileSystem::loadDirectoryContentsRecursive(std::string path)
{
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
	F_ILOG("FileSystem", "found and loaded %zu files(%.2fMB total)", files.size(), static_cast<float>(allSize) / 1000000.f);
}

MemoryBlob FileSystem::readFile(std::string path)
{
  MemoryBlob blob;
  if (fileExists(path))
  {
    auto fullPath = system_fs::path(system_fs::current_path().string() + path).string();
    blob = MemoryBlob(m_files[fullPath]);
  }
  return blob;
}

faze::MemView<const uint8_t> FileSystem::viewToFile(std::string path)
{
	faze::MemView<const uint8_t> view;
	if (fileExists(path))
	{
		auto fullPath = system_fs::path(system_fs::current_path().string() + path).string();
		view = faze::MemView<const uint8_t>(m_files[fullPath]);
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
  F_SLOG("FileSystem", "writing out file %s\n", fullPath.string().c_str());
  size_t leftToWrite = size;
  size_t offset = 0;
  while (leftToWrite > 0)
  {
    offset = fwrite(ptr + offset, sizeof(uint8_t), leftToWrite, file);
    leftToWrite -= offset;
  }

  fclose(file);
  system_fs::resize_file(fullPath, size);
  m_files[fullPath.string()] = std::move(fdata);
  return true;
}

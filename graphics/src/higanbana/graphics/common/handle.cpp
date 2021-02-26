#include "higanbana/graphics/common/handle.hpp"
#include <higanbana/core/global_debug.hpp>

namespace higanbana
{
HandlePool::HandlePool(ResourceType type, int size)
  : m_type(type)
  , m_size(static_cast<uint64_t>(size))
{
}

ResourceHandle HandlePool::allocate()
{
  if (m_freelist.empty() && m_currentSize+1 < m_size)
  {
    m_generation.push_back(0);
    auto id = m_currentSize;
    m_currentSize++;
    return ResourceHandle{id, 0, m_type, ResourceHandle::AllGpus, false};
  }
  HIGAN_ASSERT(!m_freelist.empty(), "No free handles.");
  auto id = m_freelist.back();
  m_freelist.pop_back();
  auto generation = m_generation[id]; // take current generation
  return ResourceHandle{id, generation, m_type, ResourceHandle::AllGpus, false};
}

void HandlePool::release(ResourceHandle val)
{
  HIGAN_ASSERT(val.id != ResourceHandle::InvalidId, "Invalid handle was released.");
  HIGAN_ASSERT(val.id < m_size, "Invalid handle was released.");
  HIGAN_ASSERT(val.generation == m_generation[val.id], "Invalid handle was released.");
  m_freelist.push_back(val.id);
  m_generation[val.id]++; // offset the generation to detect double free's
}

bool HandlePool::valid(ResourceHandle handle)
{
  return handle.id != ResourceHandle::InvalidId && handle.id < m_size && handle.generation == m_generation[handle.id];
}

size_t HandlePool::size() const
{
  return m_freelist.size();
}

ViewHandlePool::ViewHandlePool(ViewResourceType type, int size)
  : m_type(type)
  , m_size(static_cast<uint64_t>(size))
{
}

ViewResourceHandle ViewHandlePool::allocate()
{
  if (m_freelist.empty() && m_currentSize+1 < m_size)
  {
    m_generation.push_back(0);
    auto id = m_currentSize;
    m_currentSize++;
    return ViewResourceHandle(id, 0, m_type);
  }
  HIGAN_ASSERT(!m_freelist.empty(), "No free handles.");
  auto id = m_freelist.back();
  m_freelist.pop_back();
  auto generation = m_generation[id]; // take current generation
  return ViewResourceHandle(id, generation, m_type);
}

void ViewHandlePool::release(ViewResourceHandle val)
{
  HIGAN_ASSERT(val.id != ViewResourceHandle::InvalidViewId, "Invalid view handle was released.");
  HIGAN_ASSERT(val.id < m_size, "Invalidview handle was released.");
  HIGAN_ASSERT(val.generation == m_generation[val.id], "Invalid view handle was released.");
  m_freelist.push_back(val.id);
  m_generation[val.id]++; // offset the generation to detect double free's
}

bool ViewHandlePool::valid(ViewResourceHandle handle)
{
  return handle.id != ViewResourceHandle::InvalidViewId && handle.id < m_size && handle.generation == m_generation[handle.id];
}

size_t ViewHandlePool::size() const
{
  return m_freelist.size();
}

HandleManager::HandleManager(int poolSizes)
  : m_handleMutex(std::make_shared<std::mutex>())
{
  for (int i = 0; i < static_cast<int>(ResourceType::Count); ++i)
  {
    ResourceType type = static_cast<ResourceType>(i);
    if (type == ResourceType::Unknown)
      continue;
    m_pools.push_back(HandlePool(type, poolSizes));
  }
  for (int i = 0; i < static_cast<int>(ViewResourceType::Count); ++i)
  {
    ViewResourceType type = static_cast<ViewResourceType>(i);
    if (type == ViewResourceType::Unknown)
      continue;
    m_views.push_back(ViewHandlePool(type, poolSizes));
  }
}

ResourceHandle HandleManager::allocateResource(ResourceType type)
{
  std::lock_guard<std::mutex> lock(*m_handleMutex);
  HIGAN_ASSERT(type != ResourceType::Unknown, "please valide type.");
  int index = static_cast<int>(type) - 1;
  auto& pool = m_pools[index];
  return pool.allocate();
}

ViewResourceHandle HandleManager::allocateViewResource(ViewResourceType type, ResourceHandle resource)
{
  std::lock_guard<std::mutex> lock(*m_handleMutex);
  HIGAN_ASSERT(type != ViewResourceType::Unknown, "please valide type.");
  int index = static_cast<int>(type) - 1;
  auto& pool = m_views[index];
  auto view = pool.allocate();
  view.resource = resource.rawValue;
  return view;
}

void HandleManager::release(ResourceHandle handle)
{
  std::lock_guard<std::mutex> lock(*m_handleMutex);
  HIGAN_ASSERT(handle.type != ResourceType::Unknown, "please valied type");
  int typeIndex = static_cast<int>(handle.type) - 1;
  auto& pool = m_pools[typeIndex];
  pool.release(handle);
}

bool HandleManager::valid(ResourceHandle handle)
{
  if (handle.type == ResourceType::Unknown)
  {
    return false;
  }
  int typeIndex = static_cast<int>(handle.type);
  auto& pool = m_pools[typeIndex];
  return pool.valid(handle);
}
void HandleManager::release(ViewResourceHandle handle)
{
  std::lock_guard<std::mutex> lock(*m_handleMutex);
  HIGAN_ASSERT(handle.type != ViewResourceType::Unknown, "please valied type");
  int typeIndex = static_cast<int>(handle.type) - 1;
  auto& pool = m_views[typeIndex];
  pool.release(handle);
}

bool HandleManager::valid(ViewResourceHandle handle)
{
  if (handle.type == ViewResourceType::Unknown)
  {
    return false;
  }
  int typeIndex = static_cast<int>(handle.type);
  auto& pool = m_views[typeIndex];
  return pool.valid(handle);
}

}
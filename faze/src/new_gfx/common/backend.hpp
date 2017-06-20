#pragma once

#include "resource_descriptor.hpp"
#include <memory>
#include <vector>
#include <mutex>

namespace faze
{
  class GpuDevice;

  struct GpuHeapAllocation;

  namespace backend
  {
    struct DeviceData;

    template <typename T>
    class SharedState
    {
    protected:
      using State = T;
      using StatePtr = std::shared_ptr<T>;

      StatePtr m_state;
    public:

      template <typename... Ts>
      T &makeState(Ts &&... ts)
      {
        m_state = std::make_shared<T>(std::forward<Ts>(ts)...);
        return S();
      }

      const T &S() const { return *m_state; }
      T &S() { return *m_state; }

      bool valid() const { return !!m_state; }
    };

    struct RawView
    {
      size_t ptr;
      unsigned type;
    };
    struct TrackedState
    {
      size_t resPtr;
      size_t statePtr;
      size_t additionalInfo;
    };

    template <typename TrackedResource>
    class ResourceTracker : public std::enable_shared_from_this<ResourceTracker<TrackedResource>>
    {
      std::vector<std::shared_ptr<TrackedResource>> m_disposableObjects;
      std::vector<GpuHeapAllocation>                m_disposableMemory;
      std::mutex                                    m_lock;
    public:
      ResourceTracker()
      {
      }

      std::shared_ptr<int64_t> makeTracker(int64_t id, GpuHeapAllocation allocation, std::shared_ptr<TrackedResource> resourceToTrack)
      {
        auto dest = shared_from_this();
        return std::shared_ptr<int64_t>(new int64_t(id), [resourceToTrack, allocation, dest](int64_t* t)
        {
          std::lock_guard<std::mutex> lock(dest->m_lock);
          dest->m_disposableObjects.emplace_back(resourceToTrack);
          dest->m_disposableMemory.emplace_back(allocation);
          delete t;
        });
      }

      std::shared_ptr<int64_t> makeTracker(int64_t id, std::shared_ptr<TrackedResource> resourceToTrack)
      {
        auto dest = shared_from_this();
        return std::shared_ptr<int64_t>(new int64_t(id), [resourceToTrack, dest](int64_t* t)
        {
          std::lock_guard<std::mutex> lock(dest->m_lock);
          dest->m_disposableObjects.emplace_back(resourceToTrack);
          delete t;
        });
      }

      std::vector<std::shared_ptr<TrackedResource>> getResources()
      {
        std::lock_guard<std::mutex> lock(m_lock);
        auto vec = m_disposableObjects;
        m_disposableObjects.clear();
        return vec;
      }

      std::vector<GpuHeapAllocation> getAllocations()
      {
        std::lock_guard<std::mutex> lock(m_lock);
        auto vec = m_disposableMemory;
        m_disposableMemory.clear();
        return vec;
      }
    };

    class GpuDeviceChild
    {
      std::weak_ptr<DeviceData> m_parentDevice;
    public:
      GpuDeviceChild() = default;
      GpuDeviceChild(std::weak_ptr<DeviceData> device)
        : m_parentDevice(device)
      {}

      void setParent(GpuDevice *device);
      GpuDevice device();
    };
  }
}
#pragma once
#include "higanbana/graphics/common/handle.hpp"
#include <future>

namespace higanbana
{
  class ReadbackData
  {
    std::shared_ptr<ResourceHandle> m_id; // only id for now
    MemView<uint8_t> m_view;
  public: 
    ReadbackData()
      : m_id(std::make_shared<ResourceHandle>())
    {
    }

    ReadbackData(std::shared_ptr<ResourceHandle> id, MemView<uint8_t> view)
      : m_id(id)
      , m_view(view)
    {
    }

    template <typename T>
    MemView<T> view()
    {
      return reinterpret_memView<T, uint8_t>(m_view);
    }

    ResourceHandle handle() const
    {
      if (m_id)
        return *m_id;
      return ResourceHandle();
    }
  };

  class ReadbackFuture
  {
    std::shared_ptr<std::future<ReadbackData>> m_future;
  public:
    ReadbackFuture(){}
    ReadbackFuture(std::shared_ptr<std::future<ReadbackData>> rb)
      : m_future(rb)
    {

    }

    ReadbackData get()
    {
      return m_future->get();
    }

    void wait() const
    {
      m_future->wait();
    }

    bool ready() const
    {
      return m_future->_Is_ready();
    }
  };

  struct ReadbackPromise
  {
    std::shared_ptr<ResourceHandle> promiseId;
    std::shared_ptr<std::promise<ReadbackData>> promise;

    ReadbackFuture future()
    {
      return std::make_shared<std::future<ReadbackData>>(promise->get_future());
    }
  };
}
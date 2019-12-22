#pragma once

#include "higanbana/core/datastructures/proxy.hpp"

#include <memory>
#include <functional>

namespace higanbana
{
  template <typename T>
  class Rabbitpool // they will multiply if needed, infinite supply but never too much
  {
  private:
    std::shared_ptr<vector<T>> m_pool;
    std::function<T()> m_makeItem;
    int m_size;
  public:
    Rabbitpool()
      : m_pool(nullptr)
      , m_size(0)
    {
    }
    Rabbitpool(std::function<T()> makeItem)
      : m_pool(std::make_shared<vector<T>>())
      , m_makeItem(makeItem)
      , m_size(0)
    {
    }

    struct Item
    {
      std::weak_ptr<vector<T>> m_pool;
      T object;

      Item(T object, std::shared_ptr<vector<T>> pool)
        : m_pool(pool)
        , object(std::forward<decltype(object)>(object))
      {}

      ~Item()
      {
        if (auto pool = m_pool.lock())
        {
          pool.emplace_back(object);
        }
      }
    };

    struct SharedItem
    {
      std::shared_ptr<Item> p;

      T* operator->()
      {
        return &p->item;
      }
    };

    SharedItem allocate() noexcept
    {
      if (m_pool.empty())
      {
        m_pool->emplace_back(std::move(m_makeItem()));
        ++m_size;
      }
      auto obj = std::make_shared<Item>(m_pool->back(), m_pool);
      m_pool->pop_back();

      return obj;
    }

    void clear() noexcept
    {
      m_pool = std::make_shared<vector<T>>();
      m_size = 0;
    }
  };

  template <typename T>
  class Rabbitpool2
  {
  private:
    std::shared_ptr<vector<T>> m_pool;
    std::function<T()> m_makeItem;
    int m_size;
    std::shared_ptr<std::mutex> m_poolMutex;
  public:
    Rabbitpool2()
      : m_pool(nullptr)
      , m_size(0)
      , m_poolMutex(std::make_shared<std::mutex>())
    {
    }
    Rabbitpool2(std::function<T()> makeItem)
      : m_pool(std::make_shared<vector<T>>())
      , m_makeItem(makeItem)
      , m_size(0)
      , m_poolMutex(std::make_shared<std::mutex>())
    {
    }

    std::shared_ptr<T> allocate() noexcept
    {
      std::lock_guard<std::mutex> guard(*m_poolMutex);
      if (m_pool->empty())
      {
        m_pool->emplace_back(std::move(m_makeItem()));
        ++m_size;
      }

      std::weak_ptr<vector<T>> weakpool = m_pool;
      std::weak_ptr<std::mutex> weakpoolMutex = m_poolMutex;

      auto obj = std::shared_ptr<T>(new T(std::move(m_pool->back())), [weakpool, weakpoolMutex](T* ptr)
      {
        if (auto lock = weakpoolMutex.lock())
        {
          std::lock_guard<std::mutex> guard(*lock);
          if (auto pool = weakpool.lock())
          {
            pool->emplace_back(std::move(*ptr));
          }
        }
        delete ptr;
      });

      m_pool->pop_back();

      return obj;
    }

    void clear() noexcept
    {
      std::lock_guard<std::mutex> guard(*m_poolMutex);
      m_pool = std::make_shared<vector<T>>();
      m_size = 0;
    }

    size_t size() const noexcept
    {
      return m_pool->size();
    }

    size_t max_size() const noexcept
    {
      return m_size;
    }
  };
}
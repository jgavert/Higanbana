#pragma once

#include "core/src/datastructures/proxy.hpp"

#include <memory>
#include <functional>

namespace faze
{
  template <typename T>
  class Rabbitpool // they will multiply if needed, infinite supply but never too much
  {
  private:
    std::shared_ptr<vector<T>> m_pool;
    std::function<T()> m_makeItem;
    int size;
  public:
    Rabbitpool()
      : m_pool(nullptr)
      , size(0)
    {
    }
    Rabbitpool(std::function<T()> makeItem)
      : m_pool(std::make_shared<vector<T>>())
      , m_makeItem(makeItem)
      , size(0)
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
        ++size;
      }
      auto obj = std::make_shared<Item>(m_pool->back(), m_pool);
      m_pool->pop_back();

      return obj;
    }

    void clear() noexcept
    {
      m_pool = std::make_shared<vector<T>>();
      size = 0;
    }
  };

  template <typename T>
  class Rabbitpool2
  {
  private:
    std::shared_ptr<vector<T>> m_pool;
    std::function<T()> m_makeItem;
    int size;
  public:
    Rabbitpool2()
      : m_pool(nullptr)
      , size(0)
    {
    }
    Rabbitpool2(std::function<T()> makeItem)
      : m_pool(std::make_shared<vector<T>>())
      , m_makeItem(makeItem)
      , size(0)
    {
    }

    std::shared_ptr<T> allocate() noexcept
    {
      if (m_pool->empty())
      {
        m_pool->emplace_back(std::move(m_makeItem()));
        ++size;
      }

      std::weak_ptr<vector<T>> weakpool = m_pool;

      auto obj = std::shared_ptr<T>(new T(m_pool->back()), [weakpool](T* ptr)
      {
        if (auto pool = weakpool.lock())
        {
          pool->emplace_back(*ptr);
        }
        delete ptr;
      });

      m_pool->pop_back();

      return obj;
    }

    void clear() noexcept
    {
      m_pool = std::make_shared<vector<T>>();
      size = 0;
    }
  };
}